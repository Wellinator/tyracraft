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

  for (size_t i = 1; i < quad.vertices.size(); ++i) {
    vuMinMaxUpdate(minVec, maxVec, const_cast<float*>(quad.vertices[i].xyzw));
  }

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
  for (const Vec4& vertex : vertices) {
    vuVectorAccumulate(accum, const_cast<float*>(vertex.xyzw));
  }

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

const int Chunk::getLODFromDistance() {
  return getLODFromDistance(_distanceFromPlayerInChunks);
}

const int Chunk::getLODFromDistance(const int distance) {
  if (distance < 3)
    return 0;
  else if (distance < 4)
    return 1;
  else if (distance < 5)
    return 2;
  else
    return 3;
}

void Chunk::optimize() { mergeFaces(); }

void Chunk::mergeFaces() { quadsData.clear(); }

/**
 * @brief Merge the faces (quad of two triangles) that are
 * adjacent, faces to the same direction, has the same UV and Colors into larger
 * quads. The quad expansion must be tracked into units, and stored at span
 * properties of the quad. It's similar to greedy meshing, but in world level.
 * This optimization can reduce the number of vertices and faces to render.
 */
void Chunk::mergeFaces(std::vector<ChunkQuadData>* outQuadsData,
                       std::vector<Vec4>* inVertices,
                       std::vector<Color>* inColors, std::vector<Vec4>* inUVs) {
  if (!outQuadsData || !inVertices || !inColors || !inUVs) return;
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

  auto normalizeRange = [](s16& minVal, s16& maxVal) {
    if (maxVal < minVal) std::swap(minVal, maxVal);
    if (maxVal == minVal) ++maxVal;
  };

  for (size_t quadIdx = 0; quadIdx < quadCount; ++quadIdx) {
    FaceInfo info;
    const size_t baseIndex = quadIdx * 6;

    for (size_t v = 0; v < 6; ++v) {
      const size_t sourceIndex = baseIndex + v;
      info.quad.vertices[v] = (*inVertices)[sourceIndex];
      info.quad.colors[v] = (*inColors)[sourceIndex];
      info.quad.uv[v] = (*inUVs)[sourceIndex];
    }

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
                           const ChunkQuadData& b) -> bool {
    if (a.normal.dot3(b.normal) < 0.99F) return false;

    for (size_t i = 0; i < a.colors.size(); ++i) {
      const Color& c1 = a.colors[i];
      const Color& c2 = b.colors[i];
      if (abs(c1.r - c2.r) > 10 || abs(c1.g - c2.g) > 10 ||
          abs(c1.b - c2.b) > 10) {
        return false;
      }

      const Vec4& uv1 = a.uv[i];
      const Vec4& uv2 = b.uv[i];
      if (Utils::Abs(uv1.x - uv2.x) > 0.01F ||
          Utils::Abs(uv1.y - uv2.y) > 0.01F) {
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

    std::unordered_map<uint64_t, std::vector<size_t>> groups;
    groups.reserve(input.size());

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

      FaceInfo current = input[indices[0]];

      for (size_t pos = 1; pos < indices.size(); ++pos) {
        const FaceInfo& candidate = input[indices[pos]];
        if (getMin(candidate) == getMax(current) &&
            materialsMatch(current.quad, candidate.quad)) {
          mergeQuadPair(current.quad, candidate.quad);
          if (phase == MergePhase::AxisA) {
            current.maxA = candidate.maxA;
          } else {
            current.maxB = candidate.maxB;
          }
        } else {
          output.push_back(std::move(current));
          current = candidate;
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
  // Calculate the new span based on the direction of expansion
  Vec4 targetCenter = calculateQuadCenter(target);
  Vec4 sourceCenter = calculateQuadCenter(source);
  Vec4 direction = sourceCenter - targetCenter;

  Vec4 normal = target.normal;
  const float normalLenSq = normal.dot3(normal);
  if (Tyra::Math::equalf(normalLenSq, 0.0F, 0.00001F)) {
    computeQuadNormalVU(target);
    normal = target.normal;
  }

  const float epsilon = 0.01F;
  Vec4 minTarget, maxTarget, minSource, maxSource;
  getQuadBounds(target, minTarget, maxTarget);
  getQuadBounds(source, minSource, maxSource);

  std::array<float, 3> minA = {minTarget.x, minTarget.y, minTarget.z};
  std::array<float, 3> maxA = {maxTarget.x, maxTarget.y, maxTarget.z};
  std::array<float, 3> minB = {minSource.x, minSource.y, minSource.z};
  std::array<float, 3> maxB = {maxSource.x, maxSource.y, maxSource.z};

  auto rangesEqual = [&](int axis) {
    return Utils::Abs(minA[axis] - minB[axis]) < epsilon &&
           Utils::Abs(maxA[axis] - maxB[axis]) < epsilon;
  };

  auto rangesContiguous = [&](int axis) {
    return (Utils::Abs(maxA[axis] - minB[axis]) < epsilon) ||
           (Utils::Abs(maxB[axis] - minA[axis]) < epsilon);
  };

  auto getSpan = [&](const ChunkQuadData& quad, int axis) -> float {
    switch (axis) {
      case 0:
        return quad.span.x;
      case 1:
        return quad.span.y;
      default:
        return quad.span.z;
    }
  };

  auto setSpan = [&](ChunkQuadData& quad, int axis, float value) {
    switch (axis) {
      case 0:
        quad.span.x = value;
        break;
      case 1:
        quad.span.y = value;
        break;
      default:
        quad.span.z = value;
        break;
    }
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

  float newExpansionSpan =
      getSpan(target, expansionAxis) + getSpan(source, expansionAxis);
  float newAlignSpan =
      std::max(getSpan(target, alignAxis), getSpan(source, alignAxis));

  setSpan(target, expansionAxis, newExpansionSpan);
  setSpan(target, alignAxis, newAlignSpan);

  int remainingAxis = normalAxis;
  if (remainingAxis != expansionAxis && remainingAxis != alignAxis) {
    setSpan(target, remainingAxis,
            std::max(getSpan(target, remainingAxis),
                     getSpan(source, remainingAxis)));
  }

  // Expand the quad geometry by recalculating vertices
  expandQuadGeometry(target, source, direction, expansionAxis);
}

void Chunk::expandQuadGeometry(ChunkQuadData& target,
                               const ChunkQuadData& source,
                               const Vec4& direction, int expansionAxis) {
  static_cast<void>(direction);
  static_cast<void>(expansionAxis);
  if (target.vertices.size() != 6 || source.vertices.size() != 6) return;

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

  // Reconstruct the quad vertices to span the expanded area
  // Keep the same face orientation but expand the size
  Vec4 normal = target.normal;
  if (normal.length() == 0) {
    // Calculate normal if not set
    Vec4 v1 = target.vertices[1] - target.vertices[0];
    Vec4 v2 = target.vertices[2] - target.vertices[0];
    normal = v1.cross(v2).getNormalized();
    target.normal = normal;
  }

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

  // Keep original UV and color data - these represent the texture mapping
  // and lighting for the original block face, which should be preserved
  // The UV coordinates will be stretched across the larger face automatically
}

Vec4 Chunk::calculateQuadCenter(const ChunkQuadData& quad) {
  return computeCenterVU(quad.vertices);
}

void Chunk::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  if (quadsData.empty()) return;

  StaPipColorBag colorBag;

  StaPipTextureBag textureBag;
  textureBag.texture = BlockManager::getInstance()->getBlocksTexture();

  StaPipInfoBag infoBag;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
  infoBag.blendingEnabled = false;
  infoBag.antiAliasingEnabled = false;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  M4x4 rawMatrix = M4x4::Identity;
  infoBag.model = &rawMatrix;

  StaPipBag bag;
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  t_renderer->renderer3D.usePipeline(stapip);

  // Debug draw quads data
  for (size_t i = 0; i < quadsData.size(); i++) {
    ChunkQuadData& quad = quadsData[i];

    if (Vec4::shouldBeBackfaceCulled(&camPositon, &quad.vertices[2],
                                     &quad.vertices[1], &quad.vertices[0])) {
      continue;
    }

    /* Draw each quad's vertex mesh
    t_renderer->renderer3D.utility.drawLine(quad.vertices[0], quad.vertices[1]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[1], quad.vertices[2]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[2], quad.vertices[0]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[3], quad.vertices[4]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[4], quad.vertices[5]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[5], quad.vertices[3]);
    */

    textureBag.coordinates = quad.uv.data();
    colorBag.many = quad.colors.data();
    bag.count = 6;  // Two triangles per quad
    bag.vertices = quad.vertices.data();

    // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));

    const float d = scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;
    if (d <= 1.5f) {
      return ClippingManager_ClipAndRenderBag(&bag, stapip, t_renderer,
                                              camPositon);
    }

    stapip->core.render(&bag);
  }
};

void Chunk::rendererTransparentData(Renderer* t_renderer,
                                    StaticPipeline* stapip) {
  if (transparentQuadsData.empty()) return;

  StaPipColorBag colorBag;

  StaPipTextureBag textureBag;
  textureBag.texture = BlockManager::getInstance()->getBlocksTexture();

  StaPipInfoBag infoBag;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
  infoBag.blendingEnabled = false;
  infoBag.antiAliasingEnabled = false;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  M4x4 rawMatrix = M4x4::Identity;
  infoBag.model = &rawMatrix;

  StaPipBag bag;
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  t_renderer->renderer3D.usePipeline(stapip);

  // Debug draw quads data
  for (size_t i = 0; i < transparentQuadsData.size(); i++) {
    ChunkQuadData& quad = transparentQuadsData[i];

    if (Vec4::shouldBeBackfaceCulled(&camPositon, &quad.vertices[2],
                                     &quad.vertices[1], &quad.vertices[0])) {
      continue;
    }

    /* Draw each quad's vertex mesh
    t_renderer->renderer3D.utility.drawLine(quad.vertices[0], quad.vertices[1]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[1], quad.vertices[2]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[2], quad.vertices[0]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[3], quad.vertices[4]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[4], quad.vertices[5]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[5], quad.vertices[3]);
    */

    textureBag.coordinates = quad.uv.data();
    colorBag.many = quad.colors.data();
    bag.count = 6;  // Two triangles per quad
    bag.vertices = quad.vertices.data();

    // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));

    const float d = scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;
    if (d <= 1.5f) {
      return ClippingManager_ClipAndRenderBag(&bag, stapip, t_renderer,
                                              camPositon);
    }

    stapip->core.render(&bag);
  }
};

void Chunk::clear() {
  clearDrawData();
  state = ChunkState::Clean;
}

void Chunk::clearDrawData() {
  // vertices.clear();
  // vertices.shrink_to_fit();
  // verticesColors.clear();
  // verticesColors.shrink_to_fit();
  // uvMap.clear();
  // uvMap.shrink_to_fit();

  // verticesWithTransparency.clear();
  // verticesWithTransparency.shrink_to_fit();
  // verticesColorsWithTransparency.clear();
  // verticesColorsWithTransparency.shrink_to_fit();
  // uvMapWithTransparency.clear();
  // uvMapWithTransparency.shrink_to_fit();

  quadsData.clear();
  quadsData.shrink_to_fit();
}

void Chunk::clearDrawDataWithoutShrink() {
  // vertices.clear();
  // verticesColors.clear();
  // uvMap.clear();

  // verticesWithTransparency.clear();
  // verticesColorsWithTransparency.clear();
  // uvMapWithTransparency.clear();

  quadsData.clear();
  transparentQuadsData.clear();

  // _isDrawDataLoaded = false;
}

void Chunk::build() {
#ifdef DEBUG_MODE
  size_t initialMemoryUsage = 0;
  if (g_debug_menu.logChunkMemoryUsage) {
    initialMemoryUsage = get_used_memory();
  }

#endif  // end if DEBUG_MODE

  std::vector<Vec4> vertices;
  std::vector<Color> verticesColors;
  std::vector<Vec4> uvMap;

  // Transparency data
  std::vector<Vec4> verticesWithTransparency;
  std::vector<Color> verticesColorsWithTransparency;
  std::vector<Vec4> uvMapWithTransparency;

  for (uint16_t x = minOffset.x; x < maxOffset.x; x++) {
    for (uint16_t z = minOffset.z; z < maxOffset.z; z++) {
      for (uint16_t y = minOffset.y; y < maxOffset.y; y++) {
        Vec4 offset = Vec4(x, y, z);
        const u8 blockId = pLevel->GetBlockFromMap(x, y, z);
        const Blocks block_type = static_cast<Blocks>(blockId);

        if (blockId > (u8)Blocks::AIR_BLOCK) {
          u8 visibleFaces =
              VisibleFacesManager::getInstance()->getVisibleFacesByOffset(
                  offset);
          if (visibleFaces == 0) continue;

          Block* pBlockTemplate =
              BlockManager::getInstance()->getBlockTemplateByType(block_type);

          const bool hasTransparency = pBlockTemplate->hasTransparency();
          MeshBuilder_BuildMesh(
              &offset, visibleFaces, _lod,
              hasTransparency ? &verticesWithTransparency : &vertices,
              hasTransparency ? &verticesColorsWithTransparency
                              : &verticesColors,
              hasTransparency ? &uvMapWithTransparency : &uvMap,
              t_worldLightModel, pLevel);
        }
      }
    }
  }

  optimize();
  mergeFaces(&quadsData, &vertices, &verticesColors, &uvMap);
  mergeFaces(&transparentQuadsData, &verticesWithTransparency,
             &verticesColorsWithTransparency, &uvMapWithTransparency);

  state = ChunkState::Loaded;

#ifdef DEBUG_MODE
  if (g_debug_menu.logChunkMemoryUsage) {
    size_t totalVertices = vertices.size() + verticesWithTransparency.size();
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

void Chunk::rebuild() {
  clearDrawData();
  build();
}

bool Chunk::hasDrawData() {
  return !quadsData.empty() && !transparentQuadsData.empty();
}

void Chunk::reloadLightData() {
  /*
  verticesColors.clear();
  verticesColorsWithTransparency.clear();

  for (uint16_t x = minOffset.x; x < maxOffset.x; x++) {
    for (uint16_t z = minOffset.z; z < maxOffset.z; z++) {
      for (uint16_t y = minOffset.y; y < maxOffset.y; y++) {
        Vec4 offset = Vec4(x, y, z);
        Level* pLevel = Level::getInstance();
        const u8 blockId = pLevel->GetBlockFromMap(x, y, z);
        const Blocks block_type = static_cast<Blocks>(blockId);

        if (blockId > (u8)Blocks::AIR_BLOCK) {
          u8 visibleFaces =
              VisibleFacesManager::getInstance()->getVisibleFacesByOffset(
                  offset);

          if (visibleFaces == 0) continue;

          Block* pBlockTemplate =
              BlockManager::getInstance()->getBlockTemplateByType(block_type);

          MeshBuilder_BuildLightData(const_cast<Vec4*>(&offset), visibleFaces,
                                     pBlockTemplate->hasTransparency()
                                         ? &verticesColorsWithTransparency
                                         : &verticesColors,
                                     t_worldLightModel, pLevel);
        }
      }
    }
  }
  */
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
