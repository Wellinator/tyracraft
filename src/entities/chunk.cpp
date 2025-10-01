#include "entities/chunk.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include <cmath>
#include <array>
#include <limits>
#include <unordered_map>
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
  if (!inVertices || inVertices->empty() || !inColors || !inUVs) return;
  if (inVertices->size() != inColors->size() ||
      inVertices->size() != inUVs->size())
    return;

  // Convert triangles to quads - every 6 vertices form 2 triangles = 1 quad
  size_t numTriangles = inVertices->size() / 3;

  // Group triangles into quads (pairs of triangles)
  std::vector<bool> processed(numTriangles, false);

  for (size_t i = 0; i < numTriangles; i += 2) {
    if (i + 1 >= numTriangles) break;  // Need pairs of triangles
    if (processed[i] || processed[i + 1]) continue;

    // Create a quad from two triangles
    ChunkQuadData quad;

    // Extract vertices from two triangles (6 vertices total)
    quad.vertices.reserve(6);
    quad.colors.reserve(6);
    quad.uv.reserve(6);

    for (int j = 0; j < 6; j++) {
      size_t vertexIndex = i * 3 + j;
      if (vertexIndex < inVertices->size()) {
        quad.vertices.push_back((*inVertices)[vertexIndex]);
        quad.colors.push_back((*inColors)[vertexIndex]);
        quad.uv.push_back((*inUVs)[vertexIndex]);
      }
    }

    // Calculate and set the normal vector for this quad
    if (quad.vertices.size() >= 3) {
      Vec4 v1 = quad.vertices[1] - quad.vertices[0];
      Vec4 v2 = quad.vertices[2] - quad.vertices[0];
      quad.normal.set(v1.cross(v2));
      quad.normal.normalize();
    }

    processed[i] = true;
    processed[i + 1] = true;

    outQuadsData->push_back(std::move(quad));
  }

  // Now perform greedy meshing on the quads
  mergeAdjacentQuads(outQuadsData);
}

void Chunk::mergeAdjacentQuads(std::vector<ChunkQuadData>* quads) {
  if (!quads || quads->size() < 2) return;

  bool merged = true;
  while (merged) {
    merged = false;

    for (size_t i = 0; i < quads->size() && !merged; i++) {
      for (size_t j = i + 1; j < quads->size() && !merged; j++) {
        ChunkQuadData& quad1 = (*quads)[i];
        ChunkQuadData& quad2 = (*quads)[j];

        // Check if quads can be merged (same face direction, UV, colors)
        if (canMergeQuads(quad1, quad2)) {
          // Merge quad2 into quad1
          mergeQuadPair(quad1, quad2);

          // Remove quad2
          quads->erase(quads->begin() + j);
          merged = true;
        }
      }
    }
  }
}

bool Chunk::canMergeQuads(const ChunkQuadData& quad1,
                          const ChunkQuadData& quad2) {
  // Check if quads have same colors and UVs (simplified)
  if (quad1.colors.size() != quad2.colors.size() ||
      quad1.uv.size() != quad2.uv.size()) {
    return false;
  }

  // Check if quads face the same direction (same normal)
  Vec4 normalDiff = quad1.normal - quad2.normal;
  if (normalDiff.length() > 0.1f) {
    return false;
  }

  // Check if colors are similar (allowing for slight variations due to
  // lighting)
  for (size_t i = 0; i < quad1.colors.size(); i++) {
    const Color& c1 = quad1.colors[i];
    const Color& c2 = quad2.colors[i];
    if (abs(c1.r - c2.r) > 10 || abs(c1.g - c2.g) > 10 ||
        abs(c1.b - c2.b) > 10) {
      return false;
    }
  }

  // Check if UVs are the same
  for (size_t i = 0; i < quad1.uv.size(); i++) {
    const Vec4& uv1 = quad1.uv[i];
    const Vec4& uv2 = quad2.uv[i];
    if (abs(uv1.x - uv2.x) > 0.01f || abs(uv1.y - uv2.y) > 0.01f) {
      return false;
    }
  }

  // Check if quads are adjacent (share an edge)
  return areQuadsAdjacent(quad1, quad2);
}

bool Chunk::areQuadsAdjacent(const ChunkQuadData& quad1,
                             const ChunkQuadData& quad2) {
  const float epsilon = 0.01f;

  // First check if quads are coplanar (on the same plane)
  if (!areQuadsCoplanar(quad1, quad2, epsilon)) {
    return false;
  }

  // Get bounding boxes of both quads
  Vec4 min1, max1, min2, max2;
  getQuadBounds(quad1, min1, max1);
  getQuadBounds(quad2, min2, max2);

  std::array<float, 3> minA = {min1.x, min1.y, min1.z};
  std::array<float, 3> maxA = {max1.x, max1.y, max1.z};
  std::array<float, 3> minB = {min2.x, min2.y, min2.z};
  std::array<float, 3> maxB = {max2.x, max2.y, max2.z};

  auto rangesEqual = [&](int axis) {
    return abs(minA[axis] - minB[axis]) < epsilon &&
           abs(maxA[axis] - maxB[axis]) < epsilon;
  };

  auto rangesContiguous = [&](int axis) {
    return (abs(maxA[axis] - minB[axis]) < epsilon) ||
           (abs(maxB[axis] - minA[axis]) < epsilon);
  };

  auto rangeLength = [&](int axis) {
    return maxA[axis] - minA[axis];
  };

  auto alignAndExpand = [&](int alignAxis, int expandAxis) {
    return rangeLength(alignAxis) > epsilon &&
           rangesEqual(alignAxis) && rangesContiguous(expandAxis);
  };

  Vec4 normal = quad1.normal;
  float absX = fabs(normal.x);
  float absY = fabs(normal.y);
  float absZ = fabs(normal.z);

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

  return alignAndExpand(planeAxes[0], planeAxes[1]) ||
         alignAndExpand(planeAxes[1], planeAxes[0]);
}

bool Chunk::areQuadsCoplanar(const ChunkQuadData& quad1,
                             const ChunkQuadData& quad2, float epsilon) {
  // Check if both quads are on the same plane by comparing their distance from
  // a reference point along their normal vectors

  if (quad1.vertices.empty() || quad2.vertices.empty()) return false;

  Vec4 normal1 = quad1.normal;
  Vec4 normal2 = quad2.normal;

  // Check if normals are parallel (same direction)
  Vec4 normalDiff = normal1 - normal2;
  if (normalDiff.length() > epsilon) {
    return false;
  }

  // Check if quads are on the same plane by testing if vertices of quad2
  // are coplanar with quad1's plane
  Vec4 refPoint = quad1.vertices[0];
  float planeD = normal1.dot3(refPoint);

  for (const Vec4& vertex : quad2.vertices) {
    float distance = abs(normal1.dot3(vertex) - planeD);
    if (distance > epsilon) {
      return false;
    }
  }

  return true;
}

void Chunk::getQuadBounds(const ChunkQuadData& quad, Vec4& minBounds,
                          Vec4& maxBounds) {
  if (quad.vertices.empty()) {
    minBounds = Vec4(0, 0, 0);
    maxBounds = Vec4(0, 0, 0);
    return;
  }

  minBounds = quad.vertices[0];
  maxBounds = quad.vertices[0];

  for (const Vec4& vertex : quad.vertices) {
    minBounds.x = std::min(minBounds.x, vertex.x);
    minBounds.y = std::min(minBounds.y, vertex.y);
    minBounds.z = std::min(minBounds.z, vertex.z);
    maxBounds.x = std::max(maxBounds.x, vertex.x);
    maxBounds.y = std::max(maxBounds.y, vertex.y);
    maxBounds.z = std::max(maxBounds.z, vertex.z);
  }
}

void Chunk::mergeQuadPair(ChunkQuadData& target, const ChunkQuadData& source) {
  // Calculate the new span based on the direction of expansion
  Vec4 targetCenter = calculateQuadCenter(target);
  Vec4 sourceCenter = calculateQuadCenter(source);
  Vec4 direction = sourceCenter - targetCenter;

  Vec4 normal = target.normal;
  if (normal.length() == 0.0F) {
    Vec4 v1 = target.vertices[1] - target.vertices[0];
    Vec4 v2 = target.vertices[2] - target.vertices[0];
    normal = v1.cross(v2).getNormalized();
    target.normal = normal;
  }

  const float epsilon = 0.01f;
  Vec4 minTarget, maxTarget, minSource, maxSource;
  getQuadBounds(target, minTarget, maxTarget);
  getQuadBounds(source, minSource, maxSource);

  std::array<float, 3> minA = {minTarget.x, minTarget.y, minTarget.z};
  std::array<float, 3> maxA = {maxTarget.x, maxTarget.y, maxTarget.z};
  std::array<float, 3> minB = {minSource.x, minSource.y, minSource.z};
  std::array<float, 3> maxB = {maxSource.x, maxSource.y, maxSource.z};

  auto rangesEqual = [&](int axis) {
    return abs(minA[axis] - minB[axis]) < epsilon &&
           abs(maxA[axis] - maxB[axis]) < epsilon;
  };

  auto rangesContiguous = [&](int axis) {
    return (abs(maxA[axis] - minB[axis]) < epsilon) ||
           (abs(maxB[axis] - minA[axis]) < epsilon);
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

  float absX = fabs(normal.x);
  float absY = fabs(normal.y);
  float absZ = fabs(normal.z);

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
      expansionAxis = (fabs(direction.y) >= fabs(direction.z)) ? 1 : 2;
    } else if (normalAxis == 1) {
      expansionAxis = (fabs(direction.x) >= fabs(direction.z)) ? 0 : 2;
    } else {
      expansionAxis = (fabs(direction.x) >= fabs(direction.y)) ? 0 : 1;
    }
  }

  if (expansionAxis != planeAxes[0] && expansionAxis != planeAxes[1]) {
    expansionAxis = planeAxes[0];
  }

  int alignAxis = (planeAxes[0] == expansionAxis) ? planeAxes[1] : planeAxes[0];

  float newExpansionSpan = getSpan(target, expansionAxis) +
                           getSpan(source, expansionAxis);
  float newAlignSpan = std::max(getSpan(target, alignAxis),
                                getSpan(source, alignAxis));

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
  Vec4 minBounds = target.vertices[0];
  Vec4 maxBounds = target.vertices[0];

  // Find min/max bounds from both quads
  for (const Vec4& vertex : target.vertices) {
    minBounds.x = std::min(minBounds.x, vertex.x);
    minBounds.y = std::min(minBounds.y, vertex.y);
    minBounds.z = std::min(minBounds.z, vertex.z);
    maxBounds.x = std::max(maxBounds.x, vertex.x);
    maxBounds.y = std::max(maxBounds.y, vertex.y);
    maxBounds.z = std::max(maxBounds.z, vertex.z);
  }

  for (const Vec4& vertex : source.vertices) {
    minBounds.x = std::min(minBounds.x, vertex.x);
    minBounds.y = std::min(minBounds.y, vertex.y);
    minBounds.z = std::min(minBounds.z, vertex.z);
    maxBounds.x = std::max(maxBounds.x, vertex.x);
    maxBounds.y = std::max(maxBounds.y, vertex.y);
    maxBounds.z = std::max(maxBounds.z, vertex.z);
  }

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
  std::vector<Vec4> newVertices;
  newVertices.reserve(6);

  // Determine which face we're dealing with based on normal
  if (abs(normal.x) > 0.9f) {  // X-facing quad (left/right)
    float x = (normal.x > 0) ? maxBounds.x : minBounds.x;
    // Two triangles forming a quad on YZ plane
    newVertices.push_back(Vec4(x, minBounds.y, minBounds.z));  // Triangle 1
    newVertices.push_back(Vec4(x, maxBounds.y, minBounds.z));
    newVertices.push_back(Vec4(x, maxBounds.y, maxBounds.z));
    newVertices.push_back(Vec4(x, minBounds.y, minBounds.z));  // Triangle 2
    newVertices.push_back(Vec4(x, maxBounds.y, maxBounds.z));
    newVertices.push_back(Vec4(x, minBounds.y, maxBounds.z));
  } else if (abs(normal.y) > 0.9f) {  // Y-facing quad (top/bottom)
    float y = (normal.y > 0) ? maxBounds.y : minBounds.y;
    // Two triangles forming a quad on XZ plane
    newVertices.push_back(Vec4(minBounds.x, y, minBounds.z));  // Triangle 1
    newVertices.push_back(Vec4(maxBounds.x, y, minBounds.z));
    newVertices.push_back(Vec4(maxBounds.x, y, maxBounds.z));
    newVertices.push_back(Vec4(minBounds.x, y, minBounds.z));  // Triangle 2
    newVertices.push_back(Vec4(maxBounds.x, y, maxBounds.z));
    newVertices.push_back(Vec4(minBounds.x, y, maxBounds.z));
  } else {  // Z-facing quad (front/back)
    float z = (normal.z > 0) ? maxBounds.z : minBounds.z;
    // Two triangles forming a quad on XY plane
    newVertices.push_back(Vec4(minBounds.x, minBounds.y, z));  // Triangle 1
    newVertices.push_back(Vec4(maxBounds.x, minBounds.y, z));
    newVertices.push_back(Vec4(maxBounds.x, maxBounds.y, z));
    newVertices.push_back(Vec4(minBounds.x, minBounds.y, z));  // Triangle 2
    newVertices.push_back(Vec4(maxBounds.x, maxBounds.y, z));
    newVertices.push_back(Vec4(minBounds.x, maxBounds.y, z));
  }

  // Update target vertices with expanded geometry
  target.vertices = std::move(newVertices);

  // Keep original UV and color data - these represent the texture mapping
  // and lighting for the original block face, which should be preserved
  // The UV coordinates will be stretched across the larger face automatically
}

Vec4 Chunk::calculateQuadCenter(const ChunkQuadData& quad) {
  Vec4 center(0, 0, 0);
  if (!quad.vertices.empty()) {
    for (const Vec4& vertex : quad.vertices) {
      center = center + vertex;
    }
    center = center / static_cast<float>(quad.vertices.size());
  }
  return center;
}

void Chunk::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  if (!isLoaded() || quadsData.empty()) return;

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  // Debug draw quads data
  for (auto& quad : quadsData) {
    /* Draw each quad's vertex mesh
    t_renderer->renderer3D.utility.drawLine(quad.vertices[0], quad.vertices[1]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[1], quad.vertices[2]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[2], quad.vertices[0]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[3], quad.vertices[4]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[4], quad.vertices[5]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[5], quad.vertices[3]);
    */

    textureBag.coordinates = quad.uv.data();
    textureBag.texture = BlockManager::getInstance()->getBlocksTexture();

    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
    infoBag.blendingEnabled = false;
    infoBag.antiAliasingEnabled = false;
    infoBag.fullClipChecks = false;
    infoBag.frustumCulling =
        Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

    colorBag.many = quad.colors.data();

    bag.count = 6;  // Two triangles per quad
    bag.vertices = quad.vertices.data();
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    M4x4 rawMatrix = M4x4::Identity;
    infoBag.model = &rawMatrix;

    t_renderer->renderer3D.usePipeline(stapip);
    // // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));

    // const float distance =
    //     scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;

    // if (distance <= 1.5f) {
    //   return ClippingManager_ClipAndRenderBag(&bag, stapip, t_renderer,
    //                                           camPositon);
    // }

    stapip->core.render(&bag);
  }
};

void Chunk::rendererTransparentData(Renderer* t_renderer,
                                    StaticPipeline* stapip) {
  if (!isLoaded() || transparentQuadsData.empty()) return;

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  // Debug draw quads data
  for (auto& quad : transparentQuadsData) {
    /* Draw each quad's vertex mesh
    t_renderer->renderer3D.utility.drawLine(quad.vertices[0], quad.vertices[1]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[1], quad.vertices[2]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[2], quad.vertices[0]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[3], quad.vertices[4]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[4], quad.vertices[5]);
    t_renderer->renderer3D.utility.drawLine(quad.vertices[5], quad.vertices[3]);
    */

    textureBag.coordinates = quad.uv.data();
    textureBag.texture = BlockManager::getInstance()->getBlocksTexture();

    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
    infoBag.blendingEnabled = true;
    infoBag.antiAliasingEnabled = false;
    infoBag.fullClipChecks = false;
    infoBag.frustumCulling =
        Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

    colorBag.many = quad.colors.data();

    bag.count = 6;  // Two triangles per quad
    bag.vertices = quad.vertices.data();
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    M4x4 rawMatrix = M4x4::Identity;
    infoBag.model = &rawMatrix;

    t_renderer->renderer3D.usePipeline(stapip);
    // // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));

    // const float distance =
    //     scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;

    // if (distance <= 1.5f) {
    //   return ClippingManager_ClipAndRenderBag(&bag, stapip, t_renderer,
    //                                           camPositon);
    // }

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
