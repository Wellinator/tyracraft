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
}  // namespace

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
  
  // Ensure compressed data is freed
  std::vector<CompressedVertex>().swap(compressedVertices);
  std::vector<CompressedVertex>().swap(compressedTransparentVertices);
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
                          std::vector<Vec4>* inUVs, int offset, int count) {
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

  // Check if clipping is needed based on distance
  const float normalizedDistance =
      scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;
  if (normalizedDistance <= 1.5f) {
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
                          const std::vector<TileGroup>& groups) {
  BlockManager* blockMgr = BlockManager::getInstance();
  Tyra::Texture* tex = blockMgr->getBlocksTexture();
  t_renderer->core.texture.useTexture(tex);

  const int texW   = tex->getWidth();
  const int texH   = tex->getHeight();
  const int tileW  = texW / 16;  // texels per tile (U axis)
  const int tileH  = texH / 16;  // texels per tile (V axis)

  // Sort groups by tile to minimise CLAMP register changes.
  // Build a sorted index list to avoid copying TileGroup structs.
  std::vector<u32> sortedIdx(groups.size());
  for (u32 i = 0; i < (u32)groups.size(); ++i) sortedIdx[i] = i;
  std::sort(sortedIdx.begin(), sortedIdx.end(), [&](u32 a, u32 b) {
    // LEGACY_TILE (255) sorts last so default-wrap groups render at the end
    const TileGroup& ga = groups[a];
    const TileGroup& gb = groups[b];
    if (ga.row != gb.row) return ga.row < gb.row;
    return ga.col < gb.col;
  });

  static DmaGifBuilder clampBuilder;
  u8 prevCol = 255;
  u8 prevRow = 254;  // Distinct from any valid tile or LEGACY_TILE

  for (u32 si = 0; si < (u32)sortedIdx.size(); ++si) {
    const TileGroup& group = groups[sortedIdx[si]];

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
                  static_cast<int>(group.count));
  }

  // Restore default wrap so subsequent renders are unaffected
  dma_channel_wait(DMA_CHANNEL_VIF1, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 0);
  tex->setDefaultWrapSettings();
  sendClampRegister(clampBuilder, tex->getWrapSettings());
}

void Chunk::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  std::vector<Vec4>*  pVerts  = &vertices;
  std::vector<Vec4>*  pUV     = &UV;
  std::vector<Color>* pColors = &colors;

  if (vertices.empty() && !compressedVertices.empty()) {
    if (!decompCacheValid) {
      cachedDecompVertices.clear();
      cachedDecompColors.clear();
      cachedDecompUV.clear();
      decompressData(&cachedDecompVertices, &cachedDecompColors,
                     &cachedDecompUV, compressedVertices);
      decompCacheValid = true;
    }
    pVerts = &cachedDecompVertices;
    pColors = &cachedDecompColors;
    pUV = &cachedDecompUV;
  }

  if (pVerts->empty()) return;

  // Pre-apply fade alpha to colors once per chunk, not per draw call
  // Use a local vector (not static) to avoid accumulating memory over time
  std::vector<Color> fadedColors;
  if (fadeAlpha < 1.0f) {
    const size_t totalVerts = pColors->size();
    fadedColors.reserve(totalVerts);
    const u8 alphaValue = static_cast<u8>(fadeAlpha * 128.0f);
    for (size_t i = 0; i < totalVerts; i++) {
      Color faded = (*pColors)[i];
      faded.a = alphaValue;
      fadedColors.push_back(faded);
    }
    pColors = &fadedColors;
  }

  // Use grouped rendering if we have TileGroups AND all verts are covered by groups
  bool useGrouped = false;
  if (!mergedOpaqueGroups.empty()) {
    // Verify all vertices are covered by groups (check last group's coverage)
    const TileGroup& lastGroup = mergedOpaqueGroups.back();
    if (lastGroup.start + lastGroup.count == (u32)pVerts->size()) {
      useGrouped = true;
    }
  }

  if (useGrouped) {
    renderGrouped(t_renderer, stapip, pVerts, pColors, pUV, mergedOpaqueGroups);
  } else {
    flushDrawData(t_renderer, stapip, pVerts, pColors, pUV, 0, pVerts->size());
  }
}

void Chunk::rendererTransparentData(Renderer* t_renderer,
                                    StaticPipeline* stapip) {
  std::vector<Vec4>*  pVerts  = &transpVertices;
  std::vector<Vec4>*  pUV     = &transpUV;
  std::vector<Color>* pColors = &transpColors;

  if (transpVertices.empty() && !compressedTransparentVertices.empty()) {
    if (!decompTranspCacheValid) {
      cachedDecompTranspVertices.clear();
      cachedDecompTranspColors.clear();
      cachedDecompTranspUV.clear();
      decompressData(&cachedDecompTranspVertices, &cachedDecompTranspColors,
                     &cachedDecompTranspUV, compressedTransparentVertices);
      decompTranspCacheValid = true;
    }
    pVerts = &cachedDecompTranspVertices;
    pColors = &cachedDecompTranspColors;
    pUV = &cachedDecompTranspUV;
  }

  if (pVerts->empty()) return;

  // Pre-apply fade alpha to transparent colors once per chunk
  // Use a local vector (not static) to avoid accumulating memory over time
  std::vector<Color> fadedTranspColors;
  if (fadeAlpha < 1.0f) {
    const size_t totalVerts = pColors->size();
    fadedTranspColors.reserve(totalVerts);
    const u8 alphaValue = static_cast<u8>(fadeAlpha * 128.0f);
    for (size_t i = 0; i < totalVerts; i++) {
      Color faded = (*pColors)[i];
      faded.a = alphaValue;
      fadedTranspColors.push_back(faded);
    }
    pColors = &fadedTranspColors;
  }

  // Use grouped rendering if we have TileGroups AND all verts are covered by groups
  bool useGrouped = false;
  if (!mergedTranspGroups.empty()) {
    // Verify all vertices are covered by groups (check last group's coverage)
    const TileGroup& lastGroup = mergedTranspGroups.back();
    if (lastGroup.start + lastGroup.count == (u32)pVerts->size()) {
      useGrouped = true;
    }
  }

  if (useGrouped) {
    renderGrouped(t_renderer, stapip, pVerts, pColors, pUV, mergedTranspGroups);
  } else {
    flushDrawData(t_renderer, stapip, pVerts, pColors, pUV, 0, pVerts->size());
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

  // Invalidate decompression cache
  invalidateDecompCache();

  mergedOpaqueGroups.clear();
  mergedOpaqueGroups.shrink_to_fit();
  mergedTranspGroups.clear();
  mergedTranspGroups.shrink_to_fit();
}

void Chunk::clearDrawDataWithoutShrink() {
  vertices.clear();
  UV.clear();
  colors.clear();

  transpVertices.clear();
  transpUV.clear();
  transpColors.clear();

  // Invalidate decompression cache
  invalidateDecompCache();

  mergedOpaqueGroups.clear();
  mergedTranspGroups.clear();

  // Clear compressed data
  std::vector<CompressedVertex>().swap(compressedVertices);
  std::vector<CompressedVertex>().swap(compressedTransparentVertices);
}

void Chunk::invalidateDecompCache() {
  if (decompCacheValid) {
    cachedDecompVertices.clear();
    cachedDecompVertices.shrink_to_fit();
    cachedDecompColors.clear();
    cachedDecompColors.shrink_to_fit();
    cachedDecompUV.clear();
    cachedDecompUV.shrink_to_fit();
    decompCacheValid = false;
  }
  if (decompTranspCacheValid) {
    cachedDecompTranspVertices.clear();
    cachedDecompTranspVertices.shrink_to_fit();
    cachedDecompTranspColors.clear();
    cachedDecompTranspColors.shrink_to_fit();
    cachedDecompTranspUV.clear();
    cachedDecompTranspUV.shrink_to_fit();
    decompTranspCacheValid = false;
  }
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
        compressedVertices.size() + compressedTransparentVertices.size();
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

  meshGenFaceDir = 0;
  buildPhase     = BuildPhase::AirCheck;
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
      // Run the full BGM in one step (it is already O(n) bitwise — fast).
      buildBGM();
      buildPhase = BuildPhase::VisGraph;
      return false;
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

  compressedVertices             = std::move(output.opaqueVerts);
  mergedOpaqueGroups             = std::move(output.opaqueGroups);
  compressedTransparentVertices  = std::move(output.transpVerts);
  mergedTranspGroups             = std::move(output.transpGroups);

  // ---- Legacy pass: special-shaped blocks (torches, plants, liquids)
  // Note: slabs are now handled by BGM's processSlabs()
  VisibleFacesManager* vfm = VisibleFacesManager::getInstance();
  BlockManager*        bm  = BlockManager::getInstance();

  vertices.clear();
  UV.clear();
  colors.clear();
  transpVertices.clear();
  transpUV.clear();
  transpColors.clear();

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
      }
    }
  }

  // Pack legacy geometry into CompressedVertex and append to the BGM output.
  // Using compressData() which writes into compressedVertices etc. and then
  // clears the Vec4 temp buffers — but it overwrites, so we merge manually.
  if (!vertices.empty() || !transpVertices.empty()) {
    // Opaque legacy verts — create a TileGroup for them if any already exist
    if (!vertices.empty()) {
      u32 legacyOpaqueStart = (u32)compressedVertices.size();
      compressedVertices.reserve(compressedVertices.size() + vertices.size());
      for (size_t i = 0; i < vertices.size(); i++) {
        CompressedVertex cv;
        cv.pos.fromVec4(vertices[i]);
        packUV(UV[i], cv.u, cv.v);
        cv.color = packColor(colors[i]);
        compressedVertices.push_back(cv);
      }
      
      // If BGM created any groups, append a catch-all group for legacy vertices.
      // If no BGM groups, we'll use normal rendering (no groups).
      if (!mergedOpaqueGroups.empty()) {
        TileGroup legacyGroup;
        legacyGroup.start = legacyOpaqueStart;
        legacyGroup.count = (u32)vertices.size();
        legacyGroup.col   = TileGroup::LEGACY_TILE;
        legacyGroup.row   = TileGroup::LEGACY_TILE;
        mergedOpaqueGroups.push_back(legacyGroup);
      }
    }

    // Transparent legacy verts — create a TileGroup for them if any already exist
    if (!transpVertices.empty()) {
      u32 legacyTranspStart = (u32)compressedTransparentVertices.size();
      compressedTransparentVertices.reserve(
          compressedTransparentVertices.size() + transpVertices.size());
      for (size_t i = 0; i < transpVertices.size(); i++) {
        CompressedVertex cv;
        cv.pos.fromVec4(transpVertices[i]);
        packUV(transpUV[i], cv.u, cv.v);
        cv.color = packColor(transpColors[i]);
        compressedTransparentVertices.push_back(cv);
      }

      // If BGM created any groups, append a catch-all group for legacy vertices.
      if (!mergedTranspGroups.empty()) {
        TileGroup legacyGroup;
        legacyGroup.start = legacyTranspStart;
        legacyGroup.count = (u32)transpVertices.size();
        legacyGroup.col   = TileGroup::LEGACY_TILE;
        legacyGroup.row   = TileGroup::LEGACY_TILE;
        mergedTranspGroups.push_back(legacyGroup);
      }
    }

    // Free the temporary Vec4 buffers
    std::vector<Vec4>().swap(vertices);
    std::vector<Vec4>().swap(UV);
    std::vector<Color>().swap(colors);
    std::vector<Vec4>().swap(transpVertices);
    std::vector<Vec4>().swap(transpUV);
    std::vector<Color>().swap(transpColors);
  }
}

void Chunk::cancelBuild() {
  if (buildPhase == BuildPhase::Idle) return;

  // Release any partial geometry that was generated so far.
  clearDrawData();

  buildPhase       = BuildPhase::Idle;
  isEmpty          = false;
  state            = ChunkState::Clean;
}

bool Chunk::hasDrawData() {
  return !compressedVertices.empty() || !compressedTransparentVertices.empty();
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
  // Delegate to the full rebuild which regenerates vertices+colors together.
  reloadLightData();
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

// -----------------------------------------------------------------------------
// Compression Implementation
// -----------------------------------------------------------------------------

void Chunk::compressData() {
  if (vertices.empty() && transpVertices.empty()) return;

  // Compress opaque vertices
  compressedVertices.clear();
  compressedVertices.reserve(vertices.size()); // Reserve exact size

  for (size_t i = 0; i < vertices.size(); i++) {
    CompressedVertex cv;
    cv.pos.fromVec4(vertices[i]);
    packUV(UV[i], cv.u, cv.v);
    cv.color = packColor(colors[i]);
    compressedVertices.push_back(cv);
  }

  // Compress transparent vertices
  compressedTransparentVertices.clear();
  compressedTransparentVertices.reserve(transpVertices.size());

  for (size_t i = 0; i < transpVertices.size(); i++) {
    CompressedVertex cv;
    cv.pos.fromVec4(transpVertices[i]);
    packUV(transpUV[i], cv.u, cv.v);
    cv.color = packColor(transpColors[i]);
    compressedTransparentVertices.push_back(cv);
  }

  // Clear original data to save RAM
  // Using swap to force memory deallocation immediately
  std::vector<Vec4>().swap(vertices);
  std::vector<Vec4>().swap(UV);
  std::vector<Color>().swap(colors);

  std::vector<Vec4>().swap(transpVertices);
  std::vector<Vec4>().swap(transpUV);
  std::vector<Color>().swap(transpColors);

  // Invalidate decompression cache since compressed data changed
  invalidateDecompCache();

  // Do NOT set isCompressed = true here.
  // isCompressed tracks GEOMETRIC compression (greedy meshing),
  // whereas this method performs STORAGE compression (bit packing).
  // This prevents LOD 0 chunks (storage compressed, but geometrically raw)
  // from being flagged as low-detail and constantly rebuilt.
}

void Chunk::decompressData(std::vector<Vec4>* outVertices,
                           std::vector<Color>* outColors,
                           std::vector<Vec4>* outUV,
                           const std::vector<CompressedVertex>& inData) {
  outVertices->clear();
  outColors->clear();
  outUV->clear();

  if (inData.empty()) return;

  // Reserve to avoid reallocations during decompression
  outVertices->reserve(inData.size());
  outColors->reserve(inData.size());
  outUV->reserve(inData.size());

  for (const auto& cv : inData) {
    outVertices->emplace_back(cv.pos.toVec4());
    outColors->emplace_back(unpackColor(cv.color));
    
    Vec4 uv;
    unpackUV(uv, cv.u, cv.v);
    outUV->emplace_back(uv);
  }
}

u32 Chunk::packColor(const Color& color) {
  // Pack RGBA into 32-bit integer
  return (static_cast<u32>(color.r) << 24) |
         (static_cast<u32>(color.g) << 16) |
         (static_cast<u32>(color.b) << 8) |
         static_cast<u32>(color.a);
}

Color Chunk::unpackColor(const u32& packed) {
  return Color(
      static_cast<float>((packed >> 24) & 0xFF),
      static_cast<float>((packed >> 16) & 0xFF),
      static_cast<float>((packed >> 8) & 0xFF),
      static_cast<float>(packed & 0xFF));
}

void Chunk::packUV(const Vec4& uv, u16& u, u16& v) {
  // Scale UV by 1024.0f to preserve precision in u16
  // Range 0-64.0 becomes 0-65536
  u = static_cast<u16>(uv.x * 1024.0f);
  v = static_cast<u16>(uv.y * 1024.0f);
}

void Chunk::unpackUV(Vec4& uv, const u16& u, const u16& v) {
  uv.x = static_cast<float>(u) / 1024.0f;
  uv.y = static_cast<float>(v) / 1024.0f;
  uv.z = 1.0f;
  uv.w = 0.0f;
}
