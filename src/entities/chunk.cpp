#include "entities/chunk.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include <cmath>
#include <array>
#include <limits>
#include <unordered_map>
#include <cstdint>
#include <cstdio>

extern "C" {
#include <math3d.h>
}
#include "debug.hpp"
#include "managers/light_manager.hpp"
#include "managers/mesh/mesh_builder.hpp"
#include "managers/mesh/binary_greedy_mesher.hpp"
#include "managers/collision_manager.hpp"
#include "managers/clipping_manager.hpp"
#include "managers/particle/particle_manager.hpp"
#include "managers/particle/flame_particle.hpp"
#include "managers/particle/smoke_particle.hpp"
#include "managers/tick_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/visible_faces_manager.hpp"
#include "managers/model_builder.hpp"
#include "managers/dma_gif_builder.hpp"
#include <draw_sampling.h>
#include <gif_tags.h>

#ifdef DEBUG_MODE
#include "memory-monitor/memory_monitor.hpp"
#endif  // end if DEBUG_MODE

namespace {
constexpr float kQuadVertexCountReciprocal = 1.0F / 6.0F;

inline s16 fastFloatToS16(const float value) {
  const float bias = value >= 0.0F ? 0.5F : -0.5F;
  return static_cast<s16>(value + bias);
}

inline void vuMinMaxUpdate(float* minVec, float* maxVec, float* candidate) {
#if __GNUC__ > 3
  asm volatile(
      "lqc2 $vf1, 0x00(%2)\n"
      "lqc2 $vf2, 0x00(%0)\n"
      "lqc2 $vf3, 0x00(%1)\n"
      "vmax.xyzw $vf2, $vf2, $vf1\n"
      "vsub.xyzw $vf4, $vf0, $vf1\n"
      "vsub.xyzw $vf5, $vf0, $vf3\n"
      "vmax.xyzw $vf4, $vf4, $vf5\n"
      "vsub.xyzw $vf3, $vf0, $vf4\n"
      "sqc2 $vf2, 0x00(%0)\n"
      "sqc2 $vf3, 0x00(%1)\n"
      :
      : "r"(maxVec), "r"(minVec), "r"(candidate)
      : "memory");
#else
  asm volatile(
      "lqc2 vf1, 0x00(%2)\n"
      "lqc2 vf2, 0x00(%0)\n"
      "lqc2 vf3, 0x00(%1)\n"
      "vmax.xyzw vf2, vf2, vf1\n"
      "vsub.xyzw vf4, vf0, vf1\n"
      "vsub.xyzw vf5, vf0, vf3\n"
      "vmax.xyzw vf4, vf4, vf5\n"
      "vsub.xyzw vf3, vf0, vf4\n"
      "sqc2 vf2, 0x00(%0)\n"
      "sqc2 vf3, 0x00(%1)\n"
      :
      : "r"(maxVec), "r"(minVec), "r"(candidate)
      : "memory");
#endif
}

inline void computeQuadBoundsVU(const ChunkQuadData& quad, Vec4& minBounds,
                                Vec4& maxBounds) {
  VECTOR minVec;
  VECTOR maxVec;
  vector_copy(minVec, const_cast<float*>(quad.vertices[0].xyzw));
  vector_copy(maxVec, const_cast<float*>(quad.vertices[0].xyzw));

  // Unrolled loop for better EE pipeline - we always have exactly 6 vertices
  vuMinMaxUpdate(minVec, maxVec, const_cast<float*>(quad.vertices[1].xyzw));
  vuMinMaxUpdate(minVec, maxVec, const_cast<float*>(quad.vertices[2].xyzw));
  vuMinMaxUpdate(minVec, maxVec, const_cast<float*>(quad.vertices[3].xyzw));
  vuMinMaxUpdate(minVec, maxVec, const_cast<float*>(quad.vertices[4].xyzw));
  vuMinMaxUpdate(minVec, maxVec, const_cast<float*>(quad.vertices[5].xyzw));

  Vec4::copy(&minBounds, reinterpret_cast<const float*>(minVec));
  Vec4::copy(&maxBounds, reinterpret_cast<const float*>(maxVec));
}

inline void computeQuadNormalVU(ChunkQuadData& quad) {
  Vec4 edgeA = quad.vertices[1] - quad.vertices[0];
  Vec4 edgeB = quad.vertices[2] - quad.vertices[0];

  VECTOR edgeAVector;
  VECTOR edgeBVector;
  VECTOR normalVector;
  vector_copy(edgeAVector, edgeA.xyzw);
  vector_copy(edgeBVector, edgeB.xyzw);
  vector_cross_product(normalVector, edgeAVector, edgeBVector);
  vector_normalize(normalVector, normalVector);
  Vec4::copy(&quad.normal, reinterpret_cast<const float*>(normalVector));
  quad.normal.w = 0.0F;
}

inline void vuVectorAccumulate(float* acc, float* value) {
#if __GNUC__ > 3
  asm volatile(
      "lqc2 $vf1, 0x00(%0)\n"
      "lqc2 $vf2, 0x00(%1)\n"
      "vadd.xyzw $vf1, $vf1, $vf2\n"
      "sqc2 $vf1, 0x00(%0)\n"
      :
      : "r"(acc), "r"(value)
      : "memory");
#else
  asm volatile(
      "lqc2 vf1, 0x00(%0)\n"
      "lqc2 vf2, 0x00(%1)\n"
      "vadd.xyzw vf1, vf1, vf2\n"
      "sqc2 vf1, 0x00(%0)\n"
      :
      : "r"(acc), "r"(value)
      : "memory");
#endif
}

inline Vec4 computeCenterVU(const std::array<Vec4, 6>& vertices) {
  VECTOR accum = {0.0F, 0.0F, 0.0F, 0.0F};

  // Unrolled accumulation for 6 vertices
  vuVectorAccumulate(accum, const_cast<float*>(vertices[0].xyzw));
  vuVectorAccumulate(accum, const_cast<float*>(vertices[1].xyzw));
  vuVectorAccumulate(accum, const_cast<float*>(vertices[2].xyzw));
  vuVectorAccumulate(accum, const_cast<float*>(vertices[3].xyzw));
  vuVectorAccumulate(accum, const_cast<float*>(vertices[4].xyzw));
  vuVectorAccumulate(accum, const_cast<float*>(vertices[5].xyzw));

  Vec4 center;
  Vec4::copy(&center, reinterpret_cast<const float*>(accum));
  center *= kQuadVertexCountReciprocal;
  center.w = 1.0F;
  return center;
}

inline Color computeChunkFaceColor(Level* pLevel, WorldLightModel* lightModel,
                                   int bx, int by, int bz,
                                   BinaryGreedyMesher::FaceDir dir) {
  static constexpr float kMaxLightValue = 15.0f;
  static constexpr float kMinLightFactor = 0.15f;
  static constexpr float kFaceIntensity[6] = {0.6f, 0.6f, 1.0f,
                                              0.5f, 0.8f, 0.8f};
  static const int dx[6] = {1, -1, 0, 0, 0, 0};
  static const int dy[6] = {0, 0, 1, -1, 0, 0};
  static const int dz[6] = {0, 0, 0, 0, 1, -1};

  const int d = static_cast<int>(dir);
  const int nx = bx + dx[d];
  const int ny = by + dy[d];
  const int nz = bz + dz[d];

  u8 lightData = 0;
  if (nx >= 0 && ny >= 0 && nz >= 0 && nx < (int)pLevel->map.width &&
      ny < (int)pLevel->map.height && nz < (int)pLevel->map.length) {
    lightData = pLevel->GetLightDataFromMap((uint16_t)nx, (uint16_t)ny,
                                            (uint16_t)nz);
  }

  const u8 sunLvl = (lightData >> 4) & 0xF;
  const u8 blkLvl = lightData & 0xF;
  const float sunIntensity = lightModel ? lightModel->sunLightIntensity : 1.0f;
  const float sunFactor = std::max((sunLvl * sunIntensity) / kMaxLightValue,
                                   kMinLightFactor);
  const float blkFactor = blkLvl / kMaxLightValue;
  const float factor = std::max(sunFactor, blkFactor) * kFaceIntensity[d];

  return Color(120.0f * factor, 120.0f * factor, 120.0f * factor, 128.0f);
}
}  // namespace

bool Chunk::relightFaceSpans(
    std::vector<Color>& targetColors,
    const std::vector<BinaryGreedyMesher::LightFaceSpan>& spans) {
  if (spans.empty()) return false;

  const size_t totalColors = targetColors.size();
  for (size_t i = 0; i < spans.size(); ++i) {
    const auto& span = spans[i];
    if (span.start + 5 >= totalColors) return false;

    const auto dir = static_cast<BinaryGreedyMesher::FaceDir>(span.faceDir);
    const Color c = computeChunkFaceColor(pLevel, t_worldLightModel, span.lx,
                                          span.ly, span.lz, dir);
    for (u32 v = 0; v < 6; ++v) {
      targetColors[span.start + v] = c;
    }
  }

  return true;
}

Chunk::Chunk(const Vec4& minOffset, const Vec4& maxOffset, const u16& id) {
  this->id = id;
  this->minOffset.set(minOffset);
  this->maxOffset.set(maxOffset);
  this->center.set((maxOffset + minOffset) / 2);
  this->scaledCenterOffset.set(center * DOUBLE_BLOCK_SIZE);

  const Vec4 tempMin = minOffset * DOUBLE_BLOCK_SIZE;
  const Vec4 tempMax = maxOffset * DOUBLE_BLOCK_SIZE;
  scaledMinOffset.set(tempMin);
  scaledMaxOffset.set(tempMax);

  u32 count = 8;
  Vec4 _vertices[count] = {
      Vec4(tempMin),
      Vec4(tempMax.x, tempMin.y, tempMin.z),
      Vec4(tempMin.x, tempMax.y, tempMin.z),
      Vec4(tempMin.x, tempMin.y, tempMax.z),
      Vec4(tempMax),
      Vec4(tempMin.x, tempMax.y, tempMax.z),
      Vec4(tempMax.x, tempMin.y, tempMax.z),
      Vec4(tempMax.x, tempMax.y, tempMin.z),
  };
  this->bbox = new BBox(_vertices, count);
  
  // Initialize fade state
  fadeAlpha = 0.0f;
  isFadingIn = false;
};

Chunk::~Chunk() {
  clear();
  delete bbox;
};

void Chunk::init(Level* level, WorldLightModel* t_worldLightModel) {
  pLevel = level;
  this->t_worldLightModel = t_worldLightModel;
}

void Chunk::update(const Plane* frustumPlanes) {
  updateFrustumCheck(frustumPlanes);

  // Update fade-in animation
  if (isFadingIn && fadeAlpha < 1.0f) {
    fadeAlpha += (1.0f / FADE_IN_DURATION) * (1.0f / 60.0f);  // Assuming ~60fps
    if (fadeAlpha >= 1.0f) {
      fadeAlpha = 1.0f;
      isFadingIn = false;
    }
  }

}

void Chunk::tick() {
  for (int i = 0; i < randomTickSpeed; i++) {
    tickRandomBlock();
  }
}

void Chunk::tickRandomBlock() {
  Vec4 blockToTick = Vec4(Tyra::Math::randomi(minOffset.x, maxOffset.x),
                          Tyra::Math::randomi(minOffset.y, maxOffset.y),
                          Tyra::Math::randomi(minOffset.z, maxOffset.z));
  u8 blockType =
      pLevel->GetBlockFromMap(blockToTick.x, blockToTick.y, blockToTick.z);

  u8 emitParticles = this->_distanceFromPlayerInChunks <= 3;
  if (emitParticles) {
    u32 blockID =
        pLevel->GetPosFromXYZ(blockToTick.x, blockToTick.y, blockToTick.z);

    if (blockType == static_cast<u8>(Blocks::TORCH)) {
      // Creates smoke particle
      SmokeParticle* sp = new SmokeParticle(&blockToTick);
      ParticlesManager::EmitParticle(sp);

      // Creates Flame particle
      Particle* currentParticle = ParticlesManager::GetParticleById(blockID);

      if (currentParticle) {
        currentParticle->renew();
        return;
      }

      // Emit a flame particle
      FlameParticle* p = new FlameParticle(&blockToTick);
      p->id = blockID;
      ParticlesManager::EmitParticle(p);
    }
  }

  // Todo: add grass block spread
  // Based on https://minecraft.fandom.com/wiki/Grass_Block#Spread
}

void Chunk::setDistanceFromPlayerInChunks(const int distance) {
  this->_distanceFromPlayerInChunks = distance;
}

void Chunk::updateDistanceCache(const Vec4& playerPos) {
  // Calculate and cache squared distance (XZ plane only)
  const float dx = scaledCenterOffset.x - playerPos.x;
  const float dz = scaledCenterOffset.z - playerPos.z;
  cachedDistanceSquared = dx * dx + dz * dz;
  distanceCacheDirty = false;
}

// LOD functions removed — BGM uses a single quality level for all chunks.

void Chunk::markDirty() { dirty = true; }

void Chunk::flushDrawData(Renderer* t_renderer, StaticPipeline* stapip,
                          std::vector<Vec4>* inVertices,
                          std::vector<Color>* inColors,
                          std::vector<Vec4>* inUVs, int offset, int count,
                          bool needsClipping) {
  t_renderer->renderer3D.usePipeline(stapip);

  // Static identity matrix to avoid repeated allocation
  static M4x4 identityMatrix = M4x4::Identity;

  // Cache BlockManager instance to avoid repeated singleton lookups
  BlockManager* blockMgr = BlockManager::getInstance();

  // Colors are pre-faded by renderer()/rendererTransparentData() if needed
  StaPipColorBag colorBag;
  colorBag.many = inColors->data() + offset;

  // Initialize texture bag
  StaPipTextureBag textureBag;
  textureBag.coordinates = inUVs->data() + offset;
  textureBag.texture = blockMgr->getBlocksTexture();

  // Initialize info bag
  StaPipInfoBag infoBag;
  infoBag.model = &identityMatrix;
  infoBag.blendingEnabled = fadeAlpha < 1.0f;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;

  // Initialize main bag
  StaPipBag bag;
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;
  bag.vertices = inVertices->data() + offset;
  bag.count = static_cast<u32>(count);

  // needsClipping is pre-computed once per chunk by renderer()/renderGrouped()
  if (needsClipping) {
    ClippingManager_ClipAndRenderBag(&bag, stapip, t_renderer, camPositon);
  } else {
    stapip->core.render(&bag);
  }
}

// ---------------------------------------------------------------------------
// sendClampRegister — Send ONLY the GS CLAMP register via a minimal GIF
// packet (2 qwords).  This avoids the expensive full texture re-upload that
// updateTextureInfo() performs, and keeps the CLAMP write on the same DMA
// channel (GIF / PATH3) with proper synchronization.
// ---------------------------------------------------------------------------
static void sendClampRegister(DmaGifBuilder& builder,
                              const texwrap_t* wrap) {
  builder.begin();
  builder.addGifTag(GIF_REG_AD);
  builder.addAd(
      GS_SET_CLAMP(wrap->horizontal, wrap->vertical,
                   wrap->minu, wrap->maxu, wrap->minv, wrap->maxv),
      GS_REG_CLAMP_1);
  builder.send();
}

void Chunk::renderGrouped(Renderer* t_renderer, StaticPipeline* stapip,
                          std::vector<Vec4>* pVerts, std::vector<Color>* pColors,
                          std::vector<Vec4>* pUV,
                          const std::vector<TileGroup>& groups,
                          bool needsClipping) {
  BlockManager* blockMgr = BlockManager::getInstance();
  Tyra::Texture* tex = blockMgr->getBlocksTexture();

  const int texW  = tex->getWidth();
  const int texH  = tex->getHeight();
  const int tileW = texW / 16;  // texels per tile (U axis)
  const int tileH = texH / 16;  // texels per tile (V axis)

  // Groups must preserve buffer emission order (monotonic start/count).
  // Reordering by atlas tile breaks the mapping between TileGroup metadata
  // and the actual vertex/UV slices consumed by flushDrawData().
  static DmaGifBuilder clampBuilder;
  u8 prevCol = 255;
  u8 prevRow = 254;  // Distinct from any valid tile or LEGACY_TILE (255)

  const u32 groupCount = (u32)groups.size();
  for (u32 gi = 0; gi < groupCount; ++gi) {
    const TileGroup& group = groups[gi];

    // Only update CLAMP when the tile actually changes
    if (group.col != prevCol || group.row != prevRow) {
      // Synchronise: wait for previous VIF1 (geometry) and GIF (PATH3) DMA
      // to complete so the GS finishes rendering the prior group before we
      // change the CLAMP register.
      dma_channel_wait(DMA_CHANNEL_VIF1, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 0);

      if (group.col == TileGroup::LEGACY_TILE &&
          group.row == TileGroup::LEGACY_TILE) {
        tex->setDefaultWrapSettings();
      } else {
        const u8 col = group.col < 16 ? group.col : (u8)15u;
        const u8 row = group.row < 16 ? group.row : (u8)15u;

        const int minu = tileW - 1;
        const int maxu = col * tileW;
        const int minv = tileH - 1;
        const int maxv = row * tileH;

        tex->setWrapSettings(Tyra::TextureWrap::RegionRepeat,
                             Tyra::TextureWrap::RegionRepeat,
                             minu, minv, maxu, maxv);
      }

      // Send only the CLAMP register — no full texture re-upload
      sendClampRegister(clampBuilder, tex->getWrapSettings());

      prevCol = group.col;
      prevRow = group.row;
    }

    flushDrawData(t_renderer, stapip, pVerts, pColors, pUV,
                  static_cast<int>(group.start),
                  static_cast<int>(group.count),
                  needsClipping);
  }
  // Default-wrap restore is deferred to ChunkManager::rendererOpaque/
  // rendererTransparent to avoid one DMA wait per chunk. ChunkManager
  // performs a single restore after all chunks have been drawn.
}

void Chunk::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  if (vertices.empty()) return;

  std::vector<Vec4>*  pVerts  = &vertices;
  std::vector<Vec4>*  pUV     = &UV;
  std::vector<Color>* pColors = &colors;

  // Pre-apply fade alpha to colors once per chunk using a static buffer
  // to avoid heap allocation every frame.
  static std::vector<Color> fadedColors;
  if (fadeAlpha < 1.0f) {
    const size_t totalVerts = pColors->size();
    fadedColors.resize(totalVerts);
    const u8 alphaValue = static_cast<u8>(fadeAlpha * 128.0f);
    for (size_t i = 0; i < totalVerts; i++) {
      fadedColors[i]   = (*pColors)[i];
      fadedColors[i].a = alphaValue;
    }
    pColors = &fadedColors;
  }

  // Compute clipping flag once per chunk (not once per TileGroup/draw call).
  // Always clip when near the camera OR when the chunk is only partially inside
  // the frustum — BGM quads can span up to 16 blocks (256 world-units), so a
  // partially-visible chunk almost certainly has quads crossing a frustum plane.
  // Cap partial-frustum clipping at 3.5 chunks to avoid expensive per-triangle
  // software clipping on distant edge-of-frustum chunks.
  const float normalizedDist =
      scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;
  const bool needsClipping =
      normalizedDist <= 1.5f ||
      (frustumCheck == Tyra::CoreBBoxFrustum::PARTIALLY_IN_FRUSTUM &&
       normalizedDist <= 3.5f);

  // Use grouped rendering if we have TileGroups AND all verts are covered by groups
  bool useGrouped = false;
  if (!mergedOpaqueGroups.empty()) {
    const TileGroup& lastGroup = mergedOpaqueGroups.back();
    if (lastGroup.start + lastGroup.count == (u32)pVerts->size()) {
      useGrouped = true;
    }
  }

  if (useGrouped) {
    renderGrouped(t_renderer, stapip, pVerts, pColors, pUV,
                  mergedOpaqueGroups, needsClipping);
  } else {
    flushDrawData(t_renderer, stapip, pVerts, pColors, pUV,
                  0, pVerts->size(), needsClipping);
  }
}

void Chunk::rendererTransparentData(Renderer* t_renderer,
                                    StaticPipeline* stapip) {
  if (transpVertices.empty()) return;

  std::vector<Vec4>*  pVerts  = &transpVertices;
  std::vector<Vec4>*  pUV     = &transpUV;
  std::vector<Color>* pColors = &transpColors;

  // Pre-apply fade alpha to transparent colors once per chunk using a static
  // buffer to avoid heap allocation every frame.
  static std::vector<Color> fadedTranspColors;
  if (fadeAlpha < 1.0f) {
    const size_t totalVerts = pColors->size();
    fadedTranspColors.resize(totalVerts);
    const u8 alphaValue = static_cast<u8>(fadeAlpha * 128.0f);
    for (size_t i = 0; i < totalVerts; i++) {
      fadedTranspColors[i]   = (*pColors)[i];
      fadedTranspColors[i].a = alphaValue;
    }
    pColors = &fadedTranspColors;
  }

  // Compute clipping flag once per chunk (not once per TileGroup/draw call).
  // Always clip when near the camera OR when the chunk is only partially inside
  // the frustum — BGM quads can span up to 16 blocks (256 world-units), so a
  // partially-visible chunk almost certainly has quads crossing a frustum plane.
  // Cap partial-frustum clipping at 3.5 chunks to avoid expensive per-triangle
  // software clipping on distant edge-of-frustum chunks.
  const float normalizedDist =
      scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;
  const bool needsClipping =
      normalizedDist <= 1.5f ||
      (frustumCheck == Tyra::CoreBBoxFrustum::PARTIALLY_IN_FRUSTUM &&
       normalizedDist <= 3.5f);

  // Use grouped rendering if we have TileGroups AND all verts are covered by groups
  bool useGrouped = false;
  if (!mergedTranspGroups.empty()) {
    const TileGroup& lastGroup = mergedTranspGroups.back();
    if (lastGroup.start + lastGroup.count == (u32)pVerts->size()) {
      useGrouped = true;
    }
  }

  if (useGrouped) {
    renderGrouped(t_renderer, stapip, pVerts, pColors, pUV,
                  mergedTranspGroups, needsClipping);
  } else {
    flushDrawData(t_renderer, stapip, pVerts, pColors, pUV,
                  0, pVerts->size(), needsClipping);
  }
}

void Chunk::clear() {
  state = ChunkState::Unloading;
  clearDrawData();
  visibilityGraph = 0;
  visibilityGraphDirty = true;
  isEmpty = false;
  consecutiveOccludedFrames = 0;

  // Reset fade state
  isFadingIn = false;
  fadeAlpha = 0.0f;
  
  state = ChunkState::Clean;
  markDistanceDirty();  // Phase 1: Invalidate cache on clear
}

void Chunk::clearDrawData() {
  vertices.clear();
  vertices.shrink_to_fit();
  colors.clear();
  colors.shrink_to_fit();
  UV.clear();
  UV.shrink_to_fit();

  transpVertices.clear();
  transpVertices.shrink_to_fit();
  transpColors.clear();
  transpColors.shrink_to_fit();
  transpUV.clear();
  transpUV.shrink_to_fit();

  mergedOpaqueGroups.clear();
  mergedOpaqueGroups.shrink_to_fit();
  mergedTranspGroups.clear();
  mergedTranspGroups.shrink_to_fit();

  opaqueFaceSpans.clear();
  opaqueFaceSpans.shrink_to_fit();
  transpFaceSpans.clear();
  transpFaceSpans.shrink_to_fit();
}

void Chunk::clearDrawDataWithoutShrink() {
  vertices.clear();
  UV.clear();
  colors.clear();

  transpVertices.clear();
  transpUV.clear();
  transpColors.clear();

  mergedOpaqueGroups.clear();
  mergedTranspGroups.clear();

  opaqueFaceSpans.clear();
  transpFaceSpans.clear();
}



void Chunk::build() {
#ifdef DEBUG_MODE
  size_t initialMemoryUsage = 0;
  if (g_debug_menu.logChunkMemoryUsage) {
    initialMemoryUsage = get_used_memory();
  }
#endif

  if (state == ChunkState::Unloading) return;
  if (state == ChunkState::Loaded) return;
  if (state != ChunkState::Building) state = ChunkState::Building;

  if (dirty) {
    clearDrawDataWithoutShrink();
    dirty = false;
  }

  bool allAir = true;
  for (uint16_t y = minOffset.y; y < maxOffset.y && allAir; y++) {
    for (uint16_t z = minOffset.z; z < maxOffset.z && allAir; z++) {
      for (uint16_t x = minOffset.x; x < maxOffset.x && allAir; x++) {
        if (pLevel->GetBlockFromMap(x, y, z) > (u8)Blocks::AIR_BLOCK)
          allAir = false;
      }
    }
  }

  if (allAir) {
    clearDrawDataWithoutShrink();
    isEmpty = true;
    state   = ChunkState::Loaded;
    isFadingIn = true;
    fadeAlpha  = 0.0f;
    markDistanceDirty();
    visibilityGraph      = 0x7FFF;
    visibilityGraphDirty = false;
    if (onLoadedCallback) onLoadedCallback(this);
    return;
  }

  isEmpty = false;

  try {
    buildBGM();
    rebuildVisibilityGraph();
    state = ChunkState::Loaded;

    if (pendingIsNewChunk) {
      isFadingIn = true;
      fadeAlpha  = 0.0f;
      pendingIsNewChunk = false;
    }

    markDistanceDirty();
    if (onLoadedCallback) onLoadedCallback(this);
  } catch (...) {
    state = ChunkState::Clean;
    clearDrawData();
    throw;
  }

#ifdef DEBUG_MODE
  if (g_debug_menu.logChunkMemoryUsage) {
    const size_t totalVerts =
        vertices.size() + transpVertices.size();
    if (totalVerts > 0) {
      const size_t finalMem = get_used_memory();
      printf("Chunk %d memory: %.2f KB (%zu verts)\n", id,
             (float)(finalMem - initialMemoryUsage) / 1024.f, totalVerts);
    }
  }
#endif
}

// =============================================================================
//  INCREMENTAL BUILD PIPELINE
//  beginBuild() ──▶ buildStep() × N ──▶ done (returns true)
//  cancelBuild() can abort at any point.
// =============================================================================

void Chunk::beginBuild() {
  if (state == ChunkState::Unloading) {
    buildPhase = BuildPhase::Idle;
    return;
  }
  if (state == ChunkState::Loaded) {
    buildPhase = BuildPhase::Idle;
    return;
  }
  if (state != ChunkState::Building) {
    state = ChunkState::Building;
  }

  if (dirty) {
    clearDrawDataWithoutShrink();
    dirty = false;
  }

  // Snapshot whether this is the first time this chunk is being built.
  // Used by Finalize to decide whether to trigger the fade-in animation.
  pendingIsNewChunk = !isLoaded();

  // Reset incremental meshing state
  meshGenFaceDir = 0;
  buildPhase     = BuildPhase::AirCheck;

  // Initialise the BGM output buffers (clear + reserve).  The output
  // accumulates across multiple MeshGen buildStep() calls (one per face dir).
  static BinaryGreedyMesher bgmInit;
  bgmInit.beginMeshChunk(bgmOutput);
}

bool Chunk::buildStep() {
  switch (buildPhase) {
    // ------------------------------------------------------------------
    case BuildPhase::Idle:
      return true;

    // ------------------------------------------------------------------
    case BuildPhase::AirCheck: {
      bool allAir = true;
      for (uint16_t y = minOffset.y; y < maxOffset.y && allAir; y++) {
        for (uint16_t z = minOffset.z; z < maxOffset.z && allAir; z++) {
          for (uint16_t x = minOffset.x; x < maxOffset.x && allAir; x++) {
            if (pLevel->GetBlockFromMap(x, y, z) > (u8)Blocks::AIR_BLOCK)
              allAir = false;
          }
        }
      }
      if (allAir) {
        clearDrawDataWithoutShrink();
        isEmpty    = true;
        buildPhase = BuildPhase::Finalize;
      } else {
        isEmpty    = false;
        buildPhase = BuildPhase::MeshGen;
      }
      return false;
    }

    // ------------------------------------------------------------------
    case BuildPhase::MeshGen: {
      // Process exactly ONE face direction per buildStep() call so that
      // World::processIdleWork()'s 5 ms frame budget can interrupt between
      // directions.  A full BGM previously took 40-56 ms in one shot,
      // blocking 3-4 frames.  Now each direction takes ~6-9 ms and the
      // budget check in processIdleWork will pause after 1-2 directions.
      //
      // meshGenFaceDir 0-5 = the 6 face directions.
      // meshGenFaceDir 6   = slabs + legacy blocks + move bgmOutput -> buffers.
      static BinaryGreedyMesher bgm;

      if (meshGenFaceDir < BinaryGreedyMesher::MAX_FACE_DIRS) {
        // Process one face direction
        bgm.processFaceDir(pLevel, minOffset,
                           static_cast<BinaryGreedyMesher::FaceDir>(meshGenFaceDir),
                           t_worldLightModel, bgmOutput);
        meshGenFaceDir++;
        // Stay in MeshGen phase — more directions remain (or slabs step next)
        return false;
      } else {
        // meshGenFaceDir == 6: process slabs + legacy, then move output -> chunk
        bgm.processSlabs(pLevel, minOffset, t_worldLightModel, bgmOutput);

        // ---- Legacy pass: torches, plants, liquids (non-cuboid, non-slab)
        VisibleFacesManager* vfm = VisibleFacesManager::getInstance();
        BlockManager*        bm  = BlockManager::getInstance();
        bool hasLegacyOpaque = false;
        bool hasLegacyTransp = false;
        const u32 legacyOpaqueStart = (u32)bgmOutput.opaqueVertices.size();
        const u32 legacyTranspStart = (u32)bgmOutput.transpVertices.size();

        for (uint16_t x = minOffset.x; x < maxOffset.x; x++) {
          for (uint16_t z = minOffset.z; z < maxOffset.z; z++) {
            for (uint16_t y = minOffset.y; y < maxOffset.y; y++) {
              const u8 blockId = pLevel->GetBlockFromMap(x, y, z);
              if (blockId <= (u8)Blocks::AIR_BLOCK) continue;
              const Blocks bt = static_cast<Blocks>(blockId);
              if (BinaryGreedyMesher::isCuboidBlock(bt)) continue;
              if (BinaryGreedyMesher::isSlabBlock(bt)) continue;

              Vec4     offset(x, y, z);
              const u8 vf = vfm->getVisibleFacesByOffset(offset);
              if (!vf) continue;

              Block* tpl = bm->getBlockTemplateByType(bt);
              const bool transp = tpl->hasTransparency();

              std::vector<Vec4>*  tv = transp ? &bgmOutput.transpVertices : &bgmOutput.opaqueVertices;
              std::vector<Color>* tc = transp ? &bgmOutput.transpColors   : &bgmOutput.opaqueColors;
              std::vector<Vec4>*  tu = transp ? &bgmOutput.transpUV       : &bgmOutput.opaqueUV;

              MeshBuilder_BuildMesh(&offset, vf, 0, tv, tc, tu, t_worldLightModel, pLevel);

              if (transp) hasLegacyTransp = true;
              else        hasLegacyOpaque = true;
            }
          }
        }

        // Create TileGroups for legacy vertices if BGM groups exist
        if (hasLegacyOpaque && !bgmOutput.opaqueGroups.empty()) {
          TileGroup lg;
          lg.start = legacyOpaqueStart;
          lg.count = (u32)bgmOutput.opaqueVertices.size() - legacyOpaqueStart;
          lg.col   = TileGroup::LEGACY_TILE;
          lg.row   = TileGroup::LEGACY_TILE;
          bgmOutput.opaqueGroups.push_back(lg);
        }
        if (hasLegacyTransp && !bgmOutput.transpGroups.empty()) {
          TileGroup lg;
          lg.start = legacyTranspStart;
          lg.count = (u32)bgmOutput.transpVertices.size() - legacyTranspStart;
          lg.col   = TileGroup::LEGACY_TILE;
          lg.row   = TileGroup::LEGACY_TILE;
          bgmOutput.transpGroups.push_back(lg);
        }

        // Sort TileGroups by tile then reorder vertex/color/UV data to match.
        // Collapses interleaved same-tile groups from spatial quad emission into
        // contiguous runs, reducing CLAMP register stalls from ~100+ to ~3-8 per chunk.
        auto sortAndMergeGroups = [](
            std::vector<TileGroup>& groups,
            std::vector<Vec4>&  verts,
            std::vector<Color>& cols,
            std::vector<Vec4>&  uvs,
            std::vector<BinaryGreedyMesher::LightFaceSpan>& faceSpans) {
          if (groups.size() < 2) return;
          const size_t nGroups = groups.size();
          std::vector<size_t> order(nGroups);
          for (size_t i = 0; i < nGroups; ++i) order[i] = i;
          std::sort(order.begin(), order.end(), [&groups](size_t a, size_t b) {
            const u16 ka = (groups[a].row == TileGroup::LEGACY_TILE)
                           ? 0xFFFFu
                           : (u16)((groups[a].row << 8) | groups[a].col);
            const u16 kb = (groups[b].row == TileGroup::LEGACY_TILE)
                           ? 0xFFFFu
                           : (u16)((groups[b].row << 8) | groups[b].col);
            return ka < kb;
          });
          const size_t totalVerts = verts.size();
          std::vector<Vec4>      tmpV; tmpV.reserve(totalVerts);
          std::vector<Color>     tmpC; tmpC.reserve(totalVerts);
          std::vector<Vec4>      tmpU; tmpU.reserve(totalVerts);
          std::vector<TileGroup> sorted(nGroups);
          std::vector<BinaryGreedyMesher::LightFaceSpan> tmpSpans;
          tmpSpans.reserve(faceSpans.size());
          u32 writeOfs = 0;
          for (size_t i = 0; i < nGroups; ++i) {
            const TileGroup& src = groups[order[i]];
            sorted[i] = { writeOfs, src.count, src.col, src.row };

            for (size_t s = 0; s < faceSpans.size(); ++s) {
              const auto& span = faceSpans[s];
              if (span.start >= src.start && span.start < (src.start + src.count)) {
                auto moved = span;
                moved.start = writeOfs + (span.start - src.start);
                tmpSpans.push_back(moved);
              }
            }

            for (u32 v = src.start; v < src.start + src.count; ++v) {
              tmpV.push_back(verts[v]);
              tmpC.push_back(cols[v]);
              tmpU.push_back(uvs[v]);
            }
            writeOfs += src.count;
          }
          verts  = std::move(tmpV);
          cols   = std::move(tmpC);
          uvs    = std::move(tmpU);
          faceSpans = std::move(tmpSpans);
          groups = std::move(sorted);
          size_t write = 0;
          for (size_t i = 1; i < groups.size(); ++i) {
            if (groups[write].col == groups[i].col &&
                groups[write].row == groups[i].row) {
              groups[write].count += groups[i].count;
            } else {
              ++write;
              groups[write] = groups[i];
            }
          }
          groups.resize(write + 1);
        };
        sortAndMergeGroups(bgmOutput.opaqueGroups, bgmOutput.opaqueVertices,
               bgmOutput.opaqueColors, bgmOutput.opaqueUV,
               bgmOutput.opaqueFaceSpans);
        sortAndMergeGroups(bgmOutput.transpGroups, bgmOutput.transpVertices,
               bgmOutput.transpColors, bgmOutput.transpUV,
               bgmOutput.transpFaceSpans);

        // Move bgmOutput into the persistent chunk buffers
        vertices           = std::move(bgmOutput.opaqueVertices);
        colors             = std::move(bgmOutput.opaqueColors);
        UV                 = std::move(bgmOutput.opaqueUV);
        mergedOpaqueGroups = std::move(bgmOutput.opaqueGroups);
        opaqueFaceSpans    = std::move(bgmOutput.opaqueFaceSpans);
        transpVertices     = std::move(bgmOutput.transpVertices);
        transpColors       = std::move(bgmOutput.transpColors);
        transpUV           = std::move(bgmOutput.transpUV);
        mergedTranspGroups = std::move(bgmOutput.transpGroups);
        transpFaceSpans    = std::move(bgmOutput.transpFaceSpans);

        buildPhase = BuildPhase::VisGraph;
        return false;
      }
    }

    // ------------------------------------------------------------------
    case BuildPhase::VisGraph: {
      rebuildVisibilityGraph();
      buildPhase = BuildPhase::Finalize;
      return false;
    }

    // ------------------------------------------------------------------
    case BuildPhase::Finalize: {
      state = ChunkState::Loaded;

      if (isEmpty) {
        visibilityGraph      = 0x7FFF;
        visibilityGraphDirty = false;
      }

      if (pendingIsNewChunk) {
        isFadingIn        = true;
        fadeAlpha         = 0.0f;
        pendingIsNewChunk = false;
      }

      markDistanceDirty();
      buildPhase = BuildPhase::Idle;

      if (onLoadedCallback) onLoadedCallback(this);
      return true;
    }
  }

  return true;  // Unreachable — be safe.
}

// =============================================================================
//  buildBGM — Binary Greedy Meshing full chunk build
// =============================================================================

void Chunk::buildBGM() {
  // ---- BGM pass: cuboid blocks -----------------------------------------
  static BinaryGreedyMesher bgm;
  BinaryGreedyMesher::Output output;
  bgm.meshChunk(pLevel, minOffset, t_worldLightModel, output);

  // Move BGM results directly into persistent draw data buffers
  vertices           = std::move(output.opaqueVertices);
  colors             = std::move(output.opaqueColors);
  UV                 = std::move(output.opaqueUV);
  mergedOpaqueGroups = std::move(output.opaqueGroups);
  opaqueFaceSpans    = std::move(output.opaqueFaceSpans);

  transpVertices     = std::move(output.transpVertices);
  transpColors       = std::move(output.transpColors);
  transpUV           = std::move(output.transpUV);
  mergedTranspGroups = std::move(output.transpGroups);
  transpFaceSpans    = std::move(output.transpFaceSpans);

  // ---- Legacy pass: special-shaped blocks (torches, plants, liquids)
  // Note: slabs are now handled by BGM's processSlabs()
  VisibleFacesManager* vfm = VisibleFacesManager::getInstance();
  BlockManager*        bm  = BlockManager::getInstance();

  // Legacy blocks append directly to the persistent buffers
  bool hasLegacyOpaque = false;
  bool hasLegacyTransp = false;
  const u32 legacyOpaqueStart = (u32)vertices.size();
  const u32 legacyTranspStart = (u32)transpVertices.size();

  for (uint16_t x = minOffset.x; x < maxOffset.x; x++) {
    for (uint16_t z = minOffset.z; z < maxOffset.z; z++) {
      for (uint16_t y = minOffset.y; y < maxOffset.y; y++) {
        const u8 blockId = pLevel->GetBlockFromMap(x, y, z);
        if (blockId <= (u8)Blocks::AIR_BLOCK) continue;
        const Blocks bt = static_cast<Blocks>(blockId);
        if (BinaryGreedyMesher::isCuboidBlock(bt)) continue;
        if (BinaryGreedyMesher::isSlabBlock(bt)) continue;  // Handled by BGM

        Vec4     offset(x, y, z);
        const u8 vf = vfm->getVisibleFacesByOffset(offset);
        if (!vf) continue;

        Block*       tpl   = bm->getBlockTemplateByType(bt);
        const bool   transp = tpl->hasTransparency();

        std::vector<Vec4>*  tv = transp ? &transpVertices : &vertices;
        std::vector<Color>* tc = transp ? &transpColors   : &colors;
        std::vector<Vec4>*  tu = transp ? &transpUV       : &UV;

        MeshBuilder_BuildMesh(&offset, vf, 0, tv, tc, tu,
                              t_worldLightModel, pLevel);

        if (transp) hasLegacyTransp = true;
        else        hasLegacyOpaque = true;
      }
    }
  }

  // Create TileGroups for legacy vertices if BGM groups exist
  if (hasLegacyOpaque && !mergedOpaqueGroups.empty()) {
    TileGroup legacyGroup;
    legacyGroup.start = legacyOpaqueStart;
    legacyGroup.count = (u32)vertices.size() - legacyOpaqueStart;
    legacyGroup.col   = TileGroup::LEGACY_TILE;
    legacyGroup.row   = TileGroup::LEGACY_TILE;
    mergedOpaqueGroups.push_back(legacyGroup);
  }

  if (hasLegacyTransp && !mergedTranspGroups.empty()) {
    TileGroup legacyGroup;
    legacyGroup.start = legacyTranspStart;
    legacyGroup.count = (u32)transpVertices.size() - legacyTranspStart;
    legacyGroup.col   = TileGroup::LEGACY_TILE;
    legacyGroup.row   = TileGroup::LEGACY_TILE;
    mergedTranspGroups.push_back(legacyGroup);
  }

  // ---------------------------------------------------------------------------
  // Sort TileGroups by tile then reorder vertex/color/UV data to match.
  // Collapses interleaved same-tile groups from spatial quad emission into
  // contiguous runs, reducing CLAMP register stalls from ~100+ to ~3-8 per chunk.
  // ---------------------------------------------------------------------------
  auto sortAndMergeGroups = [](
      std::vector<TileGroup>& groups,
      std::vector<Vec4>&  verts,
      std::vector<Color>& cols,
      std::vector<Vec4>&  uvs,
      std::vector<BinaryGreedyMesher::LightFaceSpan>& faceSpans) {
    if (groups.size() < 2) return;
    const size_t nGroups = groups.size();
    std::vector<size_t> order(nGroups);
    for (size_t i = 0; i < nGroups; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&groups](size_t a, size_t b) {
      const u16 ka = (groups[a].row == TileGroup::LEGACY_TILE)
                     ? 0xFFFFu
                     : (u16)((groups[a].row << 8) | groups[a].col);
      const u16 kb = (groups[b].row == TileGroup::LEGACY_TILE)
                     ? 0xFFFFu
                     : (u16)((groups[b].row << 8) | groups[b].col);
      return ka < kb;
    });
    const size_t totalVerts = verts.size();
    std::vector<Vec4>      tmpV; tmpV.reserve(totalVerts);
    std::vector<Color>     tmpC; tmpC.reserve(totalVerts);
    std::vector<Vec4>      tmpU; tmpU.reserve(totalVerts);
    std::vector<TileGroup> sorted(nGroups);
    std::vector<BinaryGreedyMesher::LightFaceSpan> tmpSpans;
    tmpSpans.reserve(faceSpans.size());
    u32 writeOfs = 0;
    for (size_t i = 0; i < nGroups; ++i) {
      const TileGroup& src = groups[order[i]];
      sorted[i] = { writeOfs, src.count, src.col, src.row };

      for (size_t s = 0; s < faceSpans.size(); ++s) {
        const auto& span = faceSpans[s];
        if (span.start >= src.start && span.start < (src.start + src.count)) {
          auto moved = span;
          moved.start = writeOfs + (span.start - src.start);
          tmpSpans.push_back(moved);
        }
      }

      for (u32 v = src.start; v < src.start + src.count; ++v) {
        tmpV.push_back(verts[v]);
        tmpC.push_back(cols[v]);
        tmpU.push_back(uvs[v]);
      }
      writeOfs += src.count;
    }
    verts  = std::move(tmpV);
    cols   = std::move(tmpC);
    uvs    = std::move(tmpU);
    faceSpans = std::move(tmpSpans);
    groups = std::move(sorted);
    size_t write = 0;
    for (size_t i = 1; i < groups.size(); ++i) {
      if (groups[write].col == groups[i].col &&
          groups[write].row == groups[i].row) {
        groups[write].count += groups[i].count;
      } else {
        ++write;
        groups[write] = groups[i];
      }
    }
    groups.resize(write + 1);
  };
  sortAndMergeGroups(mergedOpaqueGroups, vertices, colors, UV,
                     opaqueFaceSpans);
  sortAndMergeGroups(mergedTranspGroups, transpVertices, transpColors, transpUV,
                     transpFaceSpans);
}

void Chunk::cancelBuild() {
  if (buildPhase == BuildPhase::Idle) return;

  // Release any partial geometry that was generated so far.
  clearDrawData();

  // Also release any partial BGM output accumulated during incremental meshing.
  bgmOutput.opaqueVertices.clear(); bgmOutput.opaqueColors.clear();
  bgmOutput.opaqueUV.clear();      bgmOutput.opaqueGroups.clear();
  bgmOutput.opaqueFaceSpans.clear();
  bgmOutput.transpVertices.clear(); bgmOutput.transpColors.clear();
  bgmOutput.transpUV.clear();      bgmOutput.transpGroups.clear();
  bgmOutput.transpFaceSpans.clear();

  buildPhase       = BuildPhase::Idle;
  isEmpty          = false;
  state            = ChunkState::Clean;
}

bool Chunk::hasDrawData() {
  return !vertices.empty() || !transpVertices.empty();
}

void Chunk::reloadLightData() {
  if (!isLoaded()) return;
  if (isEmpty) return;

  // BGM fully repackages colour data alongside geometry, so a full rebuild
  // is always required to update lighting.
  clearDrawDataWithoutShrink();
  state = ChunkState::Building;
  buildBGM();
  rebuildVisibilityGraph();
  state = ChunkState::Loaded;
}

void Chunk::reloadLightColorsOnly() {
  if (!isLoaded()) return;
  if (isEmpty) return;

  // If metadata is unavailable (legacy-only chunk or mismatch), fall back safely.
  bool okOpaque = true;
  bool okTransp = true;

  if (!colors.empty() && opaqueFaceSpans.empty()) okOpaque = false;
  if (!transpColors.empty() && transpFaceSpans.empty()) okTransp = false;

  if (!opaqueFaceSpans.empty()) {
    okOpaque = relightFaceSpans(colors, opaqueFaceSpans);
  }

  if (!transpFaceSpans.empty()) {
    okTransp = relightFaceSpans(transpColors, transpFaceSpans);
  }

  if (!okOpaque || !okTransp) {
    reloadLightData();
  }
}

void Chunk::rebuild() {
  // Triggered by block edits on neighbour chunks — full geometry + lighting
  // rebuild via BGM, same as reloadLightData().
  reloadLightData();
}

void Chunk::updateFrustumCheck(const Plane* frustumPlanes) {
  this->frustumPlanes = (Plane*)frustumPlanes;
  this->frustumCheck = Utils::FrustumAABBIntersect(
      frustumPlanes, &scaledMinOffset, &scaledMaxOffset);
}

u8 Chunk::containsBlock(Vec4* offset) {
  return offset->collidesBox(minOffset, maxOffset);
  // Check if the offset is within the chunk's boundaries
  // return offset->x >= minOffset.x && offset->x <= maxOffset.x &&
  //        offset->y >= minOffset.y && offset->y <= maxOffset.y &&
  //        offset->z >= minOffset.z && offset->z <= maxOffset.z;
}

void Chunk::rebuildVisibilityGraph() {
  if (isEmpty) {
    visibilityGraph = 0x7FFF;
    visibilityGraphDirty = false;
    return;
  }
  visibilityGraph = BuildVisibilityGraph(
      pLevel, static_cast<int>(minOffset.x), static_cast<int>(minOffset.y),
      static_cast<int>(minOffset.z));
  visibilityGraphDirty = false;
}

bool Chunk::isConnected(u8 faceA, u8 faceB) const {
  return IsConnected(visibilityGraph, faceA, faceB);
}

