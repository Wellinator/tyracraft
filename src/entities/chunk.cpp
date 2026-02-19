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
#include "managers/collision_manager.hpp"
#include "managers/clipping_manager.hpp"
#include "managers/particle/particle_manager.hpp"
#include "managers/particle/flame_particle.hpp"
#include "managers/particle/smoke_particle.hpp"
#include "managers/tick_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/visible_faces_manager.hpp"
#include "managers/model_builder.hpp"

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
  if (distance < 0) {
    this->_distanceFromPlayerInChunks = distance;
    return;
  }

  const int newLOD = getLODFromDistanceWithHysteresis(distance, _lod);
  if (_lod != newLOD) dirty = true;

  _lod = newLOD;
  this->_distanceFromPlayerInChunks = distance;
}

void Chunk::updateDistanceCache(const Vec4& playerPos) {
  // Calculate and cache squared distance (XZ plane only)
  const float dx = scaledCenterOffset.x - playerPos.x;
  const float dz = scaledCenterOffset.z - playerPos.z;
  cachedDistanceSquared = dx * dx + dz * dz;
  distanceCacheDirty = false;
}

const int Chunk::getLODFromDistance() {
  return getLODFromDistance(getDistanceFromPlayerInChunks());
}

const int Chunk::getLODFromDistance(const int distance) {
  if (distance < 5)
    return 0;
  else if (distance < 10)
    return 1;
  else
    return 2;
}

const int Chunk::getLODFromDistanceWithHysteresis(const int distance,
                                                  const int currentLOD) {
  switch (currentLOD) {
    case 0:
      if (distance >= 6) return (distance >= 11) ? 2 : 1;
      return 0;
    case 1:
      if (distance < 4) return 0;
      if (distance >= 11) return 2;
      return 1;
    case 2:
      if (distance < 9) return (distance < 4) ? 0 : 1;
      return 2;
    default:
      if (distance < 5) return 0;
      else if (distance < 10) return 1;
      else return 2;
  }
}

void Chunk::mergeGeometry(const u8 colorTolerance, const float uvTolerance,
                          const float normalDotThreshold,
                          const bool mergeAcrossUvs,
                          const bool includeTransparent) {
  if (vertices.empty() && transpVertices.empty()) return;

  // Process opaque geometry
  std::vector<ChunkQuadData> quadsData;
  mergeFaces(&quadsData, &vertices, &colors, &UV, colorTolerance, uvTolerance,
             normalDotThreshold, mergeAcrossUvs);

  const size_t maxOpaqueVerts = quadsData.size() * 6;
  vertices.clear();
  UV.clear();
  colors.clear();
  vertices.reserve(maxOpaqueVerts);
  UV.reserve(maxOpaqueVerts);
  colors.reserve(maxOpaqueVerts);

  // Rebuild vertex arrays from merged quads
  for (size_t i = 0; i < quadsData.size(); i++) {
    const ChunkQuadData& quad = quadsData[i];

    // Unrolled loop for better instruction pipelining on EE
    vertices.emplace_back(quad.vertices[0]);
    vertices.emplace_back(quad.vertices[1]);
    vertices.emplace_back(quad.vertices[2]);
    vertices.emplace_back(quad.vertices[3]);
    vertices.emplace_back(quad.vertices[4]);
    vertices.emplace_back(quad.vertices[5]);

    UV.emplace_back(quad.uv[0]);
    UV.emplace_back(quad.uv[1]);
    UV.emplace_back(quad.uv[2]);
    UV.emplace_back(quad.uv[3]);
    UV.emplace_back(quad.uv[4]);
    UV.emplace_back(quad.uv[5]);

    colors.emplace_back(quad.colors[0]);
    colors.emplace_back(quad.colors[1]);
    colors.emplace_back(quad.colors[2]);
    colors.emplace_back(quad.colors[3]);
    colors.emplace_back(quad.colors[4]);
    colors.emplace_back(quad.colors[5]);
  }

  if (includeTransparent) {
    std::vector<ChunkQuadData> transparentQuadsData;
    mergeFaces(&transparentQuadsData, &transpVertices, &transpColors, &transpUV,
               colorTolerance, uvTolerance, normalDotThreshold, mergeAcrossUvs);

    const size_t maxTranspVerts = transparentQuadsData.size() * 6;
    transpVertices.clear();
    transpUV.clear();
    transpColors.clear();
    transpVertices.reserve(maxTranspVerts);
    transpUV.reserve(maxTranspVerts);
    transpColors.reserve(maxTranspVerts);

    // Rebuild transparent vertex arrays from merged quads
    for (size_t i = 0; i < transparentQuadsData.size(); i++) {
      const ChunkQuadData& quad = transparentQuadsData[i];

      transpVertices.emplace_back(quad.vertices[0]);
      transpVertices.emplace_back(quad.vertices[1]);
      transpVertices.emplace_back(quad.vertices[2]);
      transpVertices.emplace_back(quad.vertices[3]);
      transpVertices.emplace_back(quad.vertices[4]);
      transpVertices.emplace_back(quad.vertices[5]);

      transpUV.emplace_back(quad.uv[0]);
      transpUV.emplace_back(quad.uv[1]);
      transpUV.emplace_back(quad.uv[2]);
      transpUV.emplace_back(quad.uv[3]);
      transpUV.emplace_back(quad.uv[4]);
      transpUV.emplace_back(quad.uv[5]);

      transpColors.emplace_back(quad.colors[0]);
      transpColors.emplace_back(quad.colors[1]);
      transpColors.emplace_back(quad.colors[2]);
      transpColors.emplace_back(quad.colors[3]);
      transpColors.emplace_back(quad.colors[4]);
      transpColors.emplace_back(quad.colors[5]);
    }
  } else {
    transpVertices.clear();
    transpUV.clear();
    transpColors.clear();
  }
}

void Chunk::compress(const u8 colorTolerance, const float uvTolerance,
                     const float normalDotThreshold,
                     const bool mergeAcrossUvs,
                     const bool includeTransparent) {
  // Safety: Don't compress if chunk is unloading or clean (no data)
  if (state == ChunkState::Unloading || state == ChunkState::Clean) {
    return;
  }

  // Safety: Don't compress if no data exists
  if (vertices.empty() && transpVertices.empty()) {
    isCompressed = true;
    return;
  }

  const bool targetUltra = mergeAcrossUvs || !includeTransparent;

  // Already compressed to requested level, nothing to do
  if (isCompressed && isUltraCompressed == targetUltra) return;

  mergeGeometry(colorTolerance, uvTolerance, normalDotThreshold,
                mergeAcrossUvs, includeTransparent);

  isCompressed = true;
  isUltraCompressed = mergeAcrossUvs || !includeTransparent;
}

void Chunk::markDirty() { dirty = true; }

// Classify a face (first triangle v0,v1,v2) into one of 6 axis-aligned groups:
// 0=TOP(+Y), 1=BOTTOM(-Y), 2=LEFT(+X), 3=RIGHT(-X), 4=FRONT(-Z), 5=BACK(+Z)
static int faceNormalGroup(const Vec4& v0, const Vec4& v1, const Vec4& v2) {
  const float ex = v1.x - v0.x, ey = v1.y - v0.y, ez = v1.z - v0.z;
  const float fx = v2.x - v0.x, fy = v2.y - v0.y, fz = v2.z - v0.z;
  const float nx = ey * fz - ez * fy;
  const float ny = ez * fx - ex * fz;
  const float nz = ex * fy - ey * fx;
  const float ax = fabsf(nx), ay = fabsf(ny), az = fabsf(nz);
  // Winding order is CW from outside, so the cross product points INWARD.
  // Signs are therefore inverted relative to the face's outward normal.
  if (ay >= ax && ay >= az) return (ny < 0.f) ? 0 : 1;  // ny<0 = inward→outward +Y = TOP
  if (ax >= az)              return (nx < 0.f) ? 2 : 3;  // nx<0 = inward→outward +X = LEFT
  return                            (nz > 0.f) ? 4 : 5;  // nz>0 = inward→outward -Z = FRONT
}

void Chunk::sortFacesByNormal(std::vector<Vec4>& verts, std::vector<Vec4>& uvs,
                              std::vector<Color>& cols, int boundaries[7]) {
  constexpr size_t stride = 6;
  const size_t faceCount = verts.size() / stride;

  std::vector<Vec4>  gv[kFaceGroupCount], gu[kFaceGroupCount];
  std::vector<Color> gc[kFaceGroupCount];

  for (size_t i = 0; i < faceCount; ++i) {
    const int g = faceNormalGroup(verts[i * stride], verts[i * stride + 1],
                                  verts[i * stride + 2]);
    for (size_t j = 0; j < stride; ++j) {
      gv[g].push_back(verts[i * stride + j]);
      gu[g].push_back(uvs[i * stride + j]);
      gc[g].push_back(cols[i * stride + j]);
    }
  }

  verts.clear(); uvs.clear(); cols.clear();
  boundaries[0] = 0;
  for (int g = 0; g < kFaceGroupCount; ++g) {
    verts.insert(verts.end(), gv[g].begin(), gv[g].end());
    uvs.insert(uvs.end(),   gu[g].begin(), gu[g].end());
    cols.insert(cols.end(), gc[g].begin(), gc[g].end());
    boundaries[g + 1] = static_cast<int>(verts.size());
  }
}

/**
 * @brief Merge the faces (quad of two triangles) that are
 * adjacent, faces to the same direction, has the same UV and Colors into larger
 * quads. The quad expansion must be tracked into units, and stored at span
 * properties of the quad. It's similar to greedy meshing, but in world level.
 * This optimization can reduce the number of vertices and faces to render.
 */
void Chunk::mergeFaces(std::vector<ChunkQuadData>* outQuadsData,
                       std::vector<Vec4>* inVertices,
                       std::vector<Color>* inColors, std::vector<Vec4>* inUVs,
                       const u8 colorTolerance, const float uvTolerance,
                       const float normalDotThreshold,
                       const bool mergeAcrossUvs) {
  // Validation: Ensure all pointers are valid
  if (!outQuadsData || !inVertices || !inColors || !inUVs) return;
  
  // Validation: Ensure input vectors have matching sizes and are not empty
  if (inVertices->empty() || inVertices->size() != inColors->size() ||
      inVertices->size() != inUVs->size())
    return;

  outQuadsData->clear();
  const size_t quadCount = inVertices->size() / 6;
  if (quadCount == 0) return;

  outQuadsData->reserve(quadCount);

  struct FaceInfo {
    ChunkQuadData quad;
    u8 orientation = 0;  // 0 = X, 1 = Y, 2 = Z
    s16 planeCoord = 0;
    s16 minA = 0;
    s16 maxA = 0;
    s16 minB = 0;
    s16 maxB = 0;
  };

  std::vector<FaceInfo> faces;
  faces.reserve(quadCount);

  // Cache raw pointers for faster access - avoids repeated vtable lookups
  const Vec4* vertData = inVertices->data();
  const Color* colorData = inColors->data();
  const Vec4* uvData = inUVs->data();

  auto normalizeRange = [](s16& minVal, s16& maxVal) {
    if (maxVal < minVal) std::swap(minVal, maxVal);
    if (maxVal == minVal) ++maxVal;
  };

  for (size_t quadIdx = 0; quadIdx < quadCount; ++quadIdx) {
    FaceInfo info;
    const size_t baseIndex = quadIdx * 6;

    // Unrolled loop for better EE pipeline - preserves vertex order for
    // backface culling
    info.quad.vertices[0] = vertData[baseIndex];
    info.quad.vertices[1] = vertData[baseIndex + 1];
    info.quad.vertices[2] = vertData[baseIndex + 2];
    info.quad.vertices[3] = vertData[baseIndex + 3];
    info.quad.vertices[4] = vertData[baseIndex + 4];
    info.quad.vertices[5] = vertData[baseIndex + 5];

    info.quad.colors[0] = colorData[baseIndex];
    info.quad.colors[1] = colorData[baseIndex + 1];
    info.quad.colors[2] = colorData[baseIndex + 2];
    info.quad.colors[3] = colorData[baseIndex + 3];
    info.quad.colors[4] = colorData[baseIndex + 4];
    info.quad.colors[5] = colorData[baseIndex + 5];

    info.quad.uv[0] = uvData[baseIndex];
    info.quad.uv[1] = uvData[baseIndex + 1];
    info.quad.uv[2] = uvData[baseIndex + 2];
    info.quad.uv[3] = uvData[baseIndex + 3];
    info.quad.uv[4] = uvData[baseIndex + 4];
    info.quad.uv[5] = uvData[baseIndex + 5];

    computeQuadNormalVU(info.quad);

    Vec4 minBounds, maxBounds;
    computeQuadBoundsVU(info.quad, minBounds, maxBounds);

    const float absX = Utils::Abs(info.quad.normal.x);
    const float absY = Utils::Abs(info.quad.normal.y);
    const float absZ = Utils::Abs(info.quad.normal.z);

    if (absX >= absY && absX >= absZ) {
      info.orientation = 0;
      info.planeCoord = fastFloatToS16((minBounds.x + maxBounds.x) * 0.5F);
      info.minA = fastFloatToS16(minBounds.y);
      info.maxA = fastFloatToS16(maxBounds.y);
      info.minB = fastFloatToS16(minBounds.z);
      info.maxB = fastFloatToS16(maxBounds.z);
    } else if (absY >= absZ) {
      info.orientation = 1;
      info.planeCoord = fastFloatToS16((minBounds.y + maxBounds.y) * 0.5F);
      info.minA = fastFloatToS16(minBounds.x);
      info.maxA = fastFloatToS16(maxBounds.x);
      info.minB = fastFloatToS16(minBounds.z);
      info.maxB = fastFloatToS16(maxBounds.z);
    } else {
      info.orientation = 2;
      info.planeCoord = fastFloatToS16((minBounds.z + maxBounds.z) * 0.5F);
      info.minA = fastFloatToS16(minBounds.x);
      info.maxA = fastFloatToS16(maxBounds.x);
      info.minB = fastFloatToS16(minBounds.y);
      info.maxB = fastFloatToS16(maxBounds.y);
    }

    normalizeRange(info.minA, info.maxA);
    normalizeRange(info.minB, info.maxB);

    faces.emplace_back(std::move(info));
  }

  if (faces.empty()) return;

  auto materialsMatch = [](const ChunkQuadData& a,
                           const ChunkQuadData& b, const u8 colorTolerance,
                           const float uvTolerance,
                           const float normalDotThreshold,
                           const bool mergeAcrossUvs) -> bool {
    if (a.normal.dot3(b.normal) < normalDotThreshold) return false;

    for (size_t i = 0; i < a.colors.size(); ++i) {
      const Color& c1 = a.colors[i];
      const Color& c2 = b.colors[i];
      if (abs(c1.r - c2.r) > colorTolerance ||
          abs(c1.g - c2.g) > colorTolerance ||
          abs(c1.b - c2.b) > colorTolerance) {
        return false;
      }

      if (mergeAcrossUvs) continue;

      const Vec4& uv1 = a.uv[i];
      const Vec4& uv2 = b.uv[i];
      if (Utils::Abs(uv1.x - uv2.x) > uvTolerance ||
          Utils::Abs(uv1.y - uv2.y) > uvTolerance) {
        return false;
      }
    }

    return true;
  };

  enum class MergePhase : uint8_t { AxisA = 0, AxisB = 1 };

  auto makeKey = [](const FaceInfo& face, MergePhase phase) -> uint64_t {
    const uint16_t plane = static_cast<uint16_t>(face.planeCoord);
    uint16_t first = 0;
    uint16_t second = 0;

    if (phase == MergePhase::AxisA) {
      first = static_cast<uint16_t>(face.minB);
      second = static_cast<uint16_t>(face.maxB);
    } else {
      first = static_cast<uint16_t>(face.minA);
      second = static_cast<uint16_t>(face.maxA);
    }

    uint64_t key = static_cast<uint64_t>(face.orientation);
    key = (key << 16) | plane;
    key = (key << 16) | first;
    key = (key << 16) | second;
    return key;
  };

  auto mergeAlongAxis = [&](std::vector<FaceInfo>& input,
                            MergePhase phase) -> std::vector<FaceInfo> {
    if (input.empty()) return {};

    // Pre-allocate with load factor consideration to avoid rehashing
    std::unordered_map<uint64_t, std::vector<size_t>> groups;
    groups.reserve(input.size() / 2);  // Heuristic: expect ~50% unique keys

    for (size_t idx = 0; idx < input.size(); ++idx) {
      groups[makeKey(input[idx], phase)].push_back(idx);
    }

    std::vector<FaceInfo> output;
    output.reserve(input.size());

    auto getMin = [&](const FaceInfo& face) -> s16 {
      return (phase == MergePhase::AxisA) ? face.minA : face.minB;
    };

    auto getMax = [&](const FaceInfo& face) -> s16 {
      return (phase == MergePhase::AxisA) ? face.maxA : face.maxB;
    };

    for (auto& entry : groups) {
      auto& indices = entry.second;
      std::sort(indices.begin(), indices.end(), [&](size_t lhs, size_t rhs) {
        return getMin(input[lhs]) < getMin(input[rhs]);
      });

      FaceInfo current = std::move(input[indices[0]]);

      for (size_t pos = 1; pos < indices.size(); ++pos) {
        FaceInfo& candidate = input[indices[pos]];
        if (getMin(candidate) == getMax(current) &&
          materialsMatch(current.quad, candidate.quad, colorTolerance,
                   uvTolerance, normalDotThreshold, mergeAcrossUvs)) {
          // Merge preserves original winding order for backface culling
          // expandQuadGeometry maintains vertex ordering
          mergeQuadPair(current.quad, candidate.quad);
          if (phase == MergePhase::AxisA) {
            current.maxA = candidate.maxA;
          } else {
            current.maxB = candidate.maxB;
          }
        } else {
          output.push_back(std::move(current));
          current = std::move(candidate);
        }
      }

      output.push_back(std::move(current));
    }

    return output;
  };

  faces = mergeAlongAxis(faces, MergePhase::AxisA);
  faces = mergeAlongAxis(faces, MergePhase::AxisB);

  outQuadsData->reserve(outQuadsData->size() + faces.size());
  for (auto& face : faces) {
    outQuadsData->push_back(std::move(face.quad));
  }
}

void Chunk::getQuadBounds(const ChunkQuadData& quad, Vec4& minBounds,
                          Vec4& maxBounds) {
  computeQuadBoundsVU(quad, minBounds, maxBounds);
}

void Chunk::mergeQuadPair(ChunkQuadData& target, const ChunkQuadData& source) {
  // Assume normal is already computed from mergeFaces - skip validation
  Vec4 normal = target.normal;

  const float epsilon = 0.01F;
  Vec4 minTarget, maxTarget, minSource, maxSource;
  getQuadBounds(target, minTarget, maxTarget);
  getQuadBounds(source, minSource, maxSource);

  // Calculate direction only for fallback heuristic
  Vec4 targetCenter = calculateQuadCenter(target);
  Vec4 sourceCenter = calculateQuadCenter(source);
  Vec4 direction = sourceCenter - targetCenter;

  std::array<float, 3> minA = {minTarget.x, minTarget.y, minTarget.z};
  std::array<float, 3> maxA = {maxTarget.x, maxTarget.y, maxTarget.z};
  std::array<float, 3> minB = {minSource.x, minSource.y, minSource.z};
  std::array<float, 3> maxB = {maxSource.x, maxSource.y, maxSource.z};

  // Inline lambdas as they're called once - reduces function call overhead
  auto rangesEqual = [&](int axis) -> bool {
    return Utils::Abs(minA[axis] - minB[axis]) < epsilon &&
           Utils::Abs(maxA[axis] - maxB[axis]) < epsilon;
  };

  auto rangesContiguous = [&](int axis) -> bool {
    return (Utils::Abs(maxA[axis] - minB[axis]) < epsilon) ||
           (Utils::Abs(maxB[axis] - minA[axis]) < epsilon);
  };

  float absX = Utils::Abs(normal.x);
  float absY = Utils::Abs(normal.y);
  float absZ = Utils::Abs(normal.z);

  int normalAxis = 0;
  if (absY > absX && absY >= absZ)
    normalAxis = 1;
  else if (absZ > absX && absZ > absY)
    normalAxis = 2;

  std::array<int, 2> planeAxes;
  if (normalAxis == 0) {
    planeAxes = {1, 2};
  } else if (normalAxis == 1) {
    planeAxes = {0, 2};
  } else {
    planeAxes = {0, 1};
  }

  int expansionAxis = -1;
  if (rangesEqual(planeAxes[0]) && rangesContiguous(planeAxes[1])) {
    expansionAxis = planeAxes[1];
  } else if (rangesEqual(planeAxes[1]) && rangesContiguous(planeAxes[0])) {
    expansionAxis = planeAxes[0];
  }

  if (expansionAxis == -1) {
    // Fallback to directional heuristic if numerical issues prevented detection
    if (normalAxis == 0) {
      expansionAxis =
          (Utils::Abs(direction.y) >= Utils::Abs(direction.z)) ? 1 : 2;
    } else if (normalAxis == 1) {
      expansionAxis =
          (Utils::Abs(direction.x) >= Utils::Abs(direction.z)) ? 0 : 2;
    } else {
      expansionAxis =
          (Utils::Abs(direction.x) >= Utils::Abs(direction.y)) ? 0 : 1;
    }
  }

  if (expansionAxis != planeAxes[0] && expansionAxis != planeAxes[1]) {
    expansionAxis = planeAxes[0];
  }

  int alignAxis = (planeAxes[0] == expansionAxis) ? planeAxes[1] : planeAxes[0];

  // Direct access to span members instead of lambda calls
  float* targetSpan = &target.span.x;
  const float* sourceSpan = &source.span.x;

  targetSpan[expansionAxis] += sourceSpan[expansionAxis];
  targetSpan[alignAxis] =
      std::max(targetSpan[alignAxis], sourceSpan[alignAxis]);

  int remainingAxis = normalAxis;
  if (remainingAxis != expansionAxis && remainingAxis != alignAxis) {
    targetSpan[remainingAxis] =
        std::max(targetSpan[remainingAxis], sourceSpan[remainingAxis]);
  }

  // Expand the quad geometry by recalculating vertices
  expandQuadGeometry(target, source, direction, expansionAxis);
}

void Chunk::expandQuadGeometry(ChunkQuadData& target,
                               const ChunkQuadData& source,
                               const Vec4& direction, int expansionAxis) {
  static_cast<void>(direction);
  static_cast<void>(expansionAxis);
  // Size check removed - guaranteed by caller in mergeFaces

  // Find the bounding box of both quads combined
  Vec4 targetMin, targetMax, sourceMin, sourceMax;
  getQuadBounds(target, targetMin, targetMax);
  getQuadBounds(source, sourceMin, sourceMax);

  Vec4 minBounds;
  Vec4 maxBounds;
  minBounds.x = std::min(targetMin.x, sourceMin.x);
  minBounds.y = std::min(targetMin.y, sourceMin.y);
  minBounds.z = std::min(targetMin.z, sourceMin.z);
  maxBounds.x = std::max(targetMax.x, sourceMax.x);
  maxBounds.y = std::max(targetMax.y, sourceMax.y);
  maxBounds.z = std::max(targetMax.z, sourceMax.z);

  // Normal is already computed and valid from mergeFaces - reuse it
  const Vec4& normal = target.normal;

  // Create new expanded quad vertices
  std::array<Vec4, 6> newVertices = {};

  // Determine which face we're dealing with based on normal
  if (Utils::Abs(normal.x) > 0.9F) {  // X-facing quad (left/right)
    float x = (normal.x > 0) ? maxBounds.x : minBounds.x;
    // Two triangles forming a quad on YZ plane
    newVertices[0] = Vec4(x, minBounds.y, minBounds.z);  // Triangle 1
    newVertices[1] = Vec4(x, maxBounds.y, minBounds.z);
    newVertices[2] = Vec4(x, maxBounds.y, maxBounds.z);
    newVertices[3] = Vec4(x, minBounds.y, minBounds.z);  // Triangle 2
    newVertices[4] = Vec4(x, maxBounds.y, maxBounds.z);
    newVertices[5] = Vec4(x, minBounds.y, maxBounds.z);
  } else if (Utils::Abs(normal.y) > 0.9F) {  // Y-facing quad (top/bottom)
    float y = (normal.y > 0) ? maxBounds.y : minBounds.y;
    // Two triangles forming a quad on XZ plane
    newVertices[0] = (Vec4(minBounds.x, y, minBounds.z));  // Triangle 1
    newVertices[1] = (Vec4(maxBounds.x, y, minBounds.z));
    newVertices[2] = (Vec4(maxBounds.x, y, maxBounds.z));
    newVertices[3] = (Vec4(minBounds.x, y, minBounds.z));  // Triangle 2
    newVertices[4] = (Vec4(maxBounds.x, y, maxBounds.z));
    newVertices[5] = (Vec4(minBounds.x, y, maxBounds.z));
  } else {  // Z-facing quad (front/back)
    float z = (normal.z > 0) ? maxBounds.z : minBounds.z;
    // Two triangles forming a quad on XY plane
    newVertices[0] = (Vec4(minBounds.x, minBounds.y, z));  // Triangle 1
    newVertices[1] = (Vec4(maxBounds.x, minBounds.y, z));
    newVertices[2] = (Vec4(maxBounds.x, maxBounds.y, z));
    newVertices[3] = (Vec4(minBounds.x, minBounds.y, z));  // Triangle 2
    newVertices[4] = (Vec4(maxBounds.x, maxBounds.y, z));
    newVertices[5] = (Vec4(minBounds.x, maxBounds.y, z));
  }

  // Update target vertices with expanded geometry
  target.vertices = std::move(newVertices);

  // Average colors from both quads to produce more representative lighting
  // for the merged face instead of arbitrarily keeping only target's colors
  for (size_t i = 0; i < 6; ++i) {
    target.colors[i].r = (target.colors[i].r + source.colors[i].r) / 2;
    target.colors[i].g = (target.colors[i].g + source.colors[i].g) / 2;
    target.colors[i].b = (target.colors[i].b + source.colors[i].b) / 2;
    // Preserve alpha channel from target
  }

  // Keep original UV data - these represent the texture mapping
  // The UV coordinates will be stretched across the larger face automatically
}

Vec4 Chunk::calculateQuadCenter(const ChunkQuadData& quad) {
  return computeCenterVU(quad.vertices);
}

void Chunk::flushDrawData(Renderer* t_renderer, StaticPipeline* stapip,
                          std::vector<Vec4>* inVertices,
                          std::vector<Color>* inColors,
                          std::vector<Vec4>* inUVs, int offset, int count) {
  t_renderer->renderer3D.usePipeline(stapip);

  // Static identity matrix to avoid repeated allocation
  static M4x4 identityMatrix = M4x4::Identity;

  // Cache BlockManager instance to avoid repeated singleton lookups
  BlockManager* blockMgr = BlockManager::getInstance();

  // Initialize color bag
  StaPipColorBag colorBag;

  // Static buffer for faded colors to avoid repeated allocations on PS2
  static std::vector<Color> fadedColors;

  // Apply fade alpha if chunk is fading
  if (fadeAlpha < 1.0f) {
    fadedColors.resize(count);
    const u8 alphaValue = static_cast<u8>(fadeAlpha * 128.0f);  // PS2 uses 0-128 for alpha
    for (int i = 0; i < count; i++) {
      fadedColors[i] = (*inColors)[offset + i];
      fadedColors[i].a = alphaValue;
    }
    colorBag.many = fadedColors.data();
  } else {
    colorBag.many = inColors->data() + offset;
  }

  // Initialize texture bag
  StaPipTextureBag textureBag;
  textureBag.coordinates = inUVs->data() + offset;
  textureBag.texture = (_lod > 0) ? blockMgr->getBlocksTextureLowRes()
                                  : blockMgr->getBlocksTexture();

  // Initialize info bag
  StaPipInfoBag infoBag;
  infoBag.model = &identityMatrix;
  infoBag.blendingEnabled = (_lod == 0) || fadeAlpha < 1.0f;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = (_lod > 0)
                            ? Tyra::PipelineShadingType::TyraShadingFlat
                            : Tyra::PipelineShadingType::TyraShadingGouraud;

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

void Chunk::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  std::vector<Vec4>*  pVerts  = &vertices;
  std::vector<Vec4>*  pUV     = &UV;
  std::vector<Color>* pColors = &colors;

  static std::vector<Vec4>  tempVertices;
  static std::vector<Color> tempColors;
  static std::vector<Vec4>  tempUV;

  if (vertices.empty() && !compressedVertices.empty()) {
    decompressData(&tempVertices, &tempColors, &tempUV, compressedVertices);
    pVerts = &tempVertices; pColors = &tempColors; pUV = &tempUV;
  }

  if (pVerts->empty()) return;

#ifdef DEBUG_MODE
  if (!g_debug_menu.enableBackfaceCulling) {
    const int totalCount = faceGroupBoundaries[kFaceGroupCount];
    if (totalCount > 0)
      flushDrawData(t_renderer, stapip, pVerts, pColors, pUV, 0, totalCount);
    return;
  }
#endif

  // Group-level back face culling: O(6) comparisons per chunk.
  // Groups: 0=TOP(+Y), 1=BOTTOM(-Y), 2=LEFT(+X), 3=RIGHT(-X), 4=FRONT(-Z), 5=BACK(+Z)
  const float px = camPositon.x, py = camPositon.y, pz = camPositon.z;
  const bool groupVisible[kFaceGroupCount] = {
    py >= scaledMinOffset.y,   // 0: TOP    +Y
    py <= scaledMaxOffset.y,   // 1: BOTTOM -Y
    px >= scaledMinOffset.x,   // 2: LEFT   +X
    px <= scaledMaxOffset.x,   // 3: RIGHT  -X
    pz <= scaledMaxOffset.z,   // 4: FRONT  -Z
    pz >= scaledMinOffset.z,   // 5: BACK   +Z
  };

  // Phase 2: Merge contiguous visible groups into a single draw call.
  // Instead of up to 6 flushDrawData() calls, emit 1-2 batched calls.
  int rangeStart = -1;
  int rangeEnd   = 0;
  for (int g = 0; g < kFaceGroupCount; ++g) {
    const int start = faceGroupBoundaries[g];
    const int end   = faceGroupBoundaries[g + 1];
    const bool hasVerts = (end - start) > 0;

    if (groupVisible[g] && hasVerts) {
      if (rangeStart < 0) rangeStart = start;  // Open a new range
      rangeEnd = end;                           // Extend range
    } else {
      if (rangeStart >= 0) {  // Flush accumulated range
        flushDrawData(t_renderer, stapip, pVerts, pColors, pUV,
                      rangeStart, rangeEnd - rangeStart);
        rangeStart = -1;
      }
    }
  }
  if (rangeStart >= 0) {  // Flush final range
    flushDrawData(t_renderer, stapip, pVerts, pColors, pUV,
                  rangeStart, rangeEnd - rangeStart);
  }
}

void Chunk::rendererTransparentData(Renderer* t_renderer,
                                    StaticPipeline* stapip) {
  std::vector<Vec4>*  pVerts  = &transpVertices;
  std::vector<Vec4>*  pUV     = &transpUV;
  std::vector<Color>* pColors = &transpColors;

  static std::vector<Vec4>  tempTranspVertices;
  static std::vector<Color> tempTranspColors;
  static std::vector<Vec4>  tempTranspUV;

  if (transpVertices.empty() && !compressedTransparentVertices.empty()) {
    decompressData(&tempTranspVertices, &tempTranspColors, &tempTranspUV,
                   compressedTransparentVertices);
    pVerts = &tempTranspVertices; pColors = &tempTranspColors; pUV = &tempTranspUV;
  }

  if (pVerts->empty()) return;

#ifdef DEBUG_MODE
  if (!g_debug_menu.enableBackfaceCulling) {
    const int totalCount = transpFaceGroupBoundaries[kFaceGroupCount];
    if (totalCount > 0)
      flushDrawData(t_renderer, stapip, pVerts, pColors, pUV, 0, totalCount);
    return;
  }
#endif

  const float px = camPositon.x, py = camPositon.y, pz = camPositon.z;
  const bool groupVisible[kFaceGroupCount] = {
    py >= scaledMinOffset.y, py <= scaledMaxOffset.y,
    px >= scaledMinOffset.x, px <= scaledMaxOffset.x,
    pz <= scaledMaxOffset.z, pz >= scaledMinOffset.z,
  };

  // Phase 2: Merge contiguous visible groups into a single draw call.
  int rangeStart = -1;
  int rangeEnd   = 0;
  for (int g = 0; g < kFaceGroupCount; ++g) {
    const int start = transpFaceGroupBoundaries[g];
    const int end   = transpFaceGroupBoundaries[g + 1];
    const bool hasVerts = (end - start) > 0;

    if (groupVisible[g] && hasVerts) {
      if (rangeStart < 0) rangeStart = start;
      rangeEnd = end;
    } else {
      if (rangeStart >= 0) {
        flushDrawData(t_renderer, stapip, pVerts, pColors, pUV,
                      rangeStart, rangeEnd - rangeStart);
        rangeStart = -1;
      }
    }
  }
  if (rangeStart >= 0) {
    flushDrawData(t_renderer, stapip, pVerts, pColors, pUV,
                  rangeStart, rangeEnd - rangeStart);
  }
}

void Chunk::clear() {
  state = ChunkState::Unloading;
  clearDrawData();
  visibilityGraph = 0;
  visibilityGraphDirty = true;
  isEmpty = false;
  isUltraCompressed = false;
  isMerged = false;
  consecutiveOccludedFrames = 0;

  // Reset fade state
  isFadingIn = false;
  fadeAlpha = 0.0f;
  isLODRebuild = false;
  
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

  isCompressed = false;
  isUltraCompressed = false;
  isMerged = false;
  _geometryLod = -1;

  std::fill(faceGroupBoundaries, faceGroupBoundaries + 7, 0);
  std::fill(transpFaceGroupBoundaries, transpFaceGroupBoundaries + 7, 0);
}

void Chunk::clearDrawDataWithoutShrink() {
  vertices.clear();
  UV.clear();
  colors.clear();

  transpVertices.clear();
  transpUV.clear();
  transpColors.clear();

  isCompressed = false;
  isUltraCompressed = false;
  isMerged = false;
  _geometryLod = -1;

  // Clear compressed data
  std::vector<CompressedVertex>().swap(compressedVertices);
  std::vector<CompressedVertex>().swap(compressedTransparentVertices);

  std::fill(faceGroupBoundaries, faceGroupBoundaries + 7, 0);
  std::fill(transpFaceGroupBoundaries, transpFaceGroupBoundaries + 7, 0);
}

void Chunk::build() {
#ifdef DEBUG_MODE
  size_t initialMemoryUsage = 0;
  if (g_debug_menu.logChunkMemoryUsage) {
    initialMemoryUsage = get_used_memory();
  }

#endif  // end if DEBUG_MODE

  // Safety: Prevent build if unloading or already loaded
  // Note: Building state is ALLOWED for new chunks
  // Loaded state is ALLOWED for LOD rebuilds (old geometry still visible)
  if (state == ChunkState::Unloading) {
    return;
  }
  if (state == ChunkState::Loaded && !isLODRebuild) {
    return;
  }

  // Transition to Building state if not already (handles direct build() calls)
  if (state != ChunkState::Building) {
    state = ChunkState::Building;
  }

  // If dirty, force complete rebuild instead of early return
  if (dirty) {
    clearDrawDataWithoutShrink();
    dirty = false;
  }

  bool allAir = true;
  for (uint16_t y = minOffset.y; y < maxOffset.y; y++) {
    for (uint16_t z = minOffset.z; z < maxOffset.z; z++) {
      for (uint16_t x = minOffset.x; x < maxOffset.x; x++) {
        const u8 blockId = pLevel->GetBlockFromMap(x, y, z);
        if (blockId > static_cast<u8>(Blocks::AIR_BLOCK)) {
          allAir = false;
          break;
        }
      }
      if (!allAir) break;
    }
    if (!allAir) break;
  }

  if (allAir) {
    clearDrawDataWithoutShrink();
    isCompressed = false;
    isUltraCompressed = false;
    isEmpty = true;
    state = ChunkState::Loaded;

    if (!isLODRebuild) {
      isFadingIn = true;
      fadeAlpha = 0.0f;
    }
    isLODRebuild = false;

    markDistanceDirty();
    visibilityGraph = 0x7FFF;
    visibilityGraphDirty = false;

    if (onLoadedCallback) onLoadedCallback(this);
    return;
  }
  isEmpty = false;

  try {
    const int lod = getLODFromDistance();
    _lod = lod;
    if (lod == 0) {
      buildMerged();
    } else if (lod == 1) {
      buildCompressed();
    } else {
      buildUltraCompressed();
    }
    _geometryLod = lod;

    state = ChunkState::Loaded;

    // Start fade-in animation only for NEW chunks (not LOD rebuilds)
    // For LOD changes, keep full opacity to avoid blink
    if (!isLODRebuild) {
      isFadingIn = true;
      fadeAlpha = 0.0f;  // Start transparent
    }
    isLODRebuild = false;  // Reset flag after use

    // Phase 1: Invalidate distance cache after build
    markDistanceDirty();

    // Build visibility graph for cave culling
    rebuildVisibilityGraph();

    // Only storage-compress LOD 1+ chunks.
    // LOD 0 keeps uncompressed data in memory to avoid per-frame decompression.
    if (_lod > 0) {
      compressData();
    }

    // Notify that chunk is ready for lighting updates
    if (onLoadedCallback) onLoadedCallback(this);
  } catch (...) {
    // On error, revert to clean state and clear partial data
    state = ChunkState::Clean;
    clearDrawData();
    throw;  // Re-throw to let caller handle error
  }

#ifdef DEBUG_MODE
  if (g_debug_menu.logChunkMemoryUsage) {
    size_t totalVertices = vertices.size() + transpVertices.size();
    if (totalVertices > 0) {
      size_t finalMemoryUsage = get_used_memory();
      float memoryUsage =
          static_cast<float>(finalMemoryUsage - initialMemoryUsage) / 1024.0f;
      printf("Chunk %d memory usage: %.2f KB (Total of vertices: %d)\n", id,
             memoryUsage, totalVertices);
    }
  }
#endif  // end if DEBUG_MODE
}

void Chunk::buildNormaly() {
  // Cache singleton instances to avoid repeated lookups
  VisibleFacesManager* visibleFacesMgr = VisibleFacesManager::getInstance();
  BlockManager* blockMgr = BlockManager::getInstance();

  // Y as outer loop provides better cache locality for vertical column access
  // This matches how the level data is typically accessed
  for (uint16_t x = minOffset.x; x < maxOffset.x; x++) {
    for (uint16_t z = minOffset.z; z < maxOffset.z; z++) {
      for (uint16_t y = minOffset.y; y < maxOffset.y; y++) {
        const u8 blockId = pLevel->GetBlockFromMap(x, y, z);

        // Early exit for air blocks - most common case
        if (blockId <= (u8)Blocks::AIR_BLOCK) continue;

        Vec4 offset(x, y, z);
        const u8 visibleFaces =
            visibleFacesMgr->getVisibleFacesByOffset(offset);

        // Skip blocks with no visible faces
        if (visibleFaces == 0) continue;

        const Blocks block_type = static_cast<Blocks>(blockId);
        Block* pBlockTemplate = blockMgr->getBlockTemplateByType(block_type);
        const bool hasTransparency = pBlockTemplate->hasTransparency();

        // Select appropriate buffers based on transparency
        std::vector<Vec4>* targetVertices =
            hasTransparency ? &transpVertices : &vertices;
        std::vector<Color>* targetColors =
            hasTransparency ? &transpColors : &colors;
        std::vector<Vec4>* targetUV = hasTransparency ? &transpUV : &UV;

        MeshBuilder_BuildMesh(&offset, visibleFaces, _lod, targetVertices,
                              targetColors, targetUV, t_worldLightModel,
                              pLevel);
      }
    }
  }
};

void Chunk::buildMerged() {
  buildNormaly();
  // LOD 0 (near chunks): No geometry merging. UV mapping must stay correct.
  // Greedy meshing is only applied at LOD 2 (distant chunks) where UV
  // stretching is acceptable and not noticeable at that distance.
  if (!vertices.empty())
    sortFacesByNormal(vertices, UV, colors, faceGroupBoundaries);
  if (!transpVertices.empty())
    sortFacesByNormal(transpVertices, transpUV, transpColors, transpFaceGroupBoundaries);
  isMerged = false;
}

void Chunk::buildCompressed() {
  buildNormaly();
  // LOD 1 (medium distance): No geometry merging, UV stays correct.
  // Storage compression (compressData) is applied in build() for _lod > 0.
  if (!vertices.empty())
    sortFacesByNormal(vertices, UV, colors, faceGroupBoundaries);
  if (!transpVertices.empty())
    sortFacesByNormal(transpVertices, transpUV, transpColors, transpFaceGroupBoundaries);
  isUltraCompressed = false;
};

void Chunk::buildUltraCompressed() {
  buildNormaly();
  compress(15, 0.1f, 0.95f, true, false);  // Tightened colorTolerance from 30 to 15
  // Sort AFTER compress() because mergeGeometry() reshuffles faces
  if (!vertices.empty())
    sortFacesByNormal(vertices, UV, colors, faceGroupBoundaries);
  if (!transpVertices.empty())
    sortFacesByNormal(transpVertices, transpUV, transpColors, transpFaceGroupBoundaries);
  isUltraCompressed = true;
}

void Chunk::rebuild() {
  // Preserve fade state - this chunk is already visible, no blink needed
  const float savedFadeAlpha = fadeAlpha;
  const bool wasFadingIn = isFadingIn;

  clearDrawDataWithoutShrink();
  // Mark as LOD rebuild to tell build() not to reset fade
  // (build() already allows Loaded state through when isLODRebuild is true)
  isLODRebuild = true;
  build();

  // Restore fade state so the chunk doesn't flash transparent
  if (savedFadeAlpha >= 1.0f) {
    fadeAlpha = 1.0f;
    isFadingIn = false;
  } else {
    fadeAlpha = savedFadeAlpha;
    isFadingIn = wasFadingIn;
  }
}

void Chunk::updateLOD() {
  if (!isLoaded()) return;  // Safety check

  // Safety: prevent updateLOD during building or unloading
  if (state == ChunkState::Building || state == ChunkState::Unloading) return;

  const int currentLOD = getLODFromDistance();

  // LOD 0 = near: no geometry merge, full-res, uncompressed in RAM
  // LOD 1 = mid:  no geometry merge, UV correct, storage compressed
  // LOD 2 = far:  greedy mesh (UV stretching OK at distance), storage compressed
  //
  // isCompressed is TRUE only for LOD 2 (the only LOD that applies greedy meshing).
  // LOD 0 and LOD 1 share the same unmerged geometry; transitions between them
  // don't require a geometry rebuild.

  if (currentLOD <= 1 && isCompressed) {
    // Transitioning from LOD 2 → LOD 0/1: merged geometry must be discarded
    dirty = true;
  } else if (currentLOD == 2 && !isCompressed) {
    // Transitioning to LOD 2: need to apply greedy meshing
    if (!vertices.empty() || !transpVertices.empty()) {
      // LOD 0 has uncompressed vertices in RAM - can compress in-place
      compress(15, 0.1f, 0.95f, true, false);
      if (!vertices.empty())
        sortFacesByNormal(vertices, UV, colors, faceGroupBoundaries);
      if (!transpVertices.empty())
        sortFacesByNormal(transpVertices, transpUV, transpColors, transpFaceGroupBoundaries);
      _geometryLod = 2;
      compressData();
    } else {
      // LOD 1 is storage-compressed (no vertices in RAM) - force full rebuild
      dirty = true;
    }
  }
  // LOD 0 ↔ LOD 1: geometry is identical (unmerged), no rebuild needed.
  // Visual difference (Gouraud vs Flat shading, texture resolution) is
  // handled at render time via _lod in flushDrawData().
}

bool Chunk::hasDrawData() {
  return !vertices.empty() && !transpVertices.empty();
}

void Chunk::reloadLightData() {
  if (!isLoaded()) return;
  if (isEmpty) return;

  // For compressed chunks, we need to rebuild geometry then recompress
  // because greedy meshing restructures vertex arrays - color indices
  // won't align with merged vertices if we just regenerate colors
  if (isCompressed) {
    // Save compression state
    const bool wasUltraCompressed = isUltraCompressed;

    // Clear and rebuild geometry with fresh lighting
    clearDrawDataWithoutShrink();
    buildNormaly();

    // Recompress with appropriate settings
    if (wasUltraCompressed) {
      compress(15, 0.1f, 0.95f, true, false);
      isUltraCompressed = true;
    } else {
      compress(5, 0.01f, 0.99f, false, true);
      isUltraCompressed = false;
    }

    isCompressed = true;

    if (!vertices.empty())
      sortFacesByNormal(vertices, UV, colors, faceGroupBoundaries);
    if (!transpVertices.empty())
      sortFacesByNormal(transpVertices, transpUV, transpColors, transpFaceGroupBoundaries);

    // Re-compress storage after geometry rebuild
    compressData();
    return;
  }

  // For LOD 0 merged chunks, vertex arrays are restructured by merging,
  // so we must rebuild + re-merge to update lighting correctly
  if (isMerged) {
    clearDrawDataWithoutShrink();
    buildMerged();
    // LOD 0 stays uncompressed - no compressData() call
    return;
  }

  // If not geometrically compressed (LOD 0) but STORAGE compressed (packed),
  // we must rebuild to update lighting because we don't have unpacked colors.
  if (!compressedVertices.empty()) {
    clearDrawDataWithoutShrink();
    buildNormaly();
    if (!vertices.empty())
      sortFacesByNormal(vertices, UV, colors, faceGroupBoundaries);
    if (!transpVertices.empty())
      sortFacesByNormal(transpVertices, transpUV, transpColors, transpFaceGroupBoundaries);
    compressData();
    return;
  }

  // For uncompressed chunks (LOD 0, not storage-compressed): full rebuild to
  // keep color indices aligned with sorted vertex order.
  clearDrawDataWithoutShrink();
  buildNormaly();
  if (!vertices.empty())
    sortFacesByNormal(vertices, UV, colors, faceGroupBoundaries);
  if (!transpVertices.empty())
    sortFacesByNormal(transpVertices, transpUV, transpColors, transpFaceGroupBoundaries);
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
  
  std::vector<Color>().swap(transpColors);
  
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
