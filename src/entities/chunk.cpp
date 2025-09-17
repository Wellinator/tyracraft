#include "entities/chunk.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
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
  resetLoadingOffset();

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

void Chunk::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  if (isLoaded()) {
    if (vertices.empty()) return;

    StaPipTextureBag textureBag;
    StaPipInfoBag infoBag;
    StaPipColorBag colorBag;
    StaPipBag bag;

    textureBag.coordinates = uvMap.data();
    textureBag.texture = BlockManager::getInstance()->getBlocksTexture();

    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
    infoBag.blendingEnabled = true;
    infoBag.antiAliasingEnabled = false;
    infoBag.fullClipChecks = false;
    infoBag.frustumCulling =
        Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

    colorBag.many = verticesColors.data();

    bag.count = vertices.size();
    bag.vertices = vertices.data();
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    t_renderer->renderer3D.usePipeline(stapip);

    M4x4 rawMatrix = M4x4::Identity;
    infoBag.model = &rawMatrix;

    // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));

    const float distance =
        scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;

    if (distance <= 1.5f) {
      return ClippingManager_ClipAndRenderBag(&bag, stapip, t_renderer,
                                              camPositon);
    }

    stapip->core.render(&bag);
  }
};

void Chunk::rendererTransparentData(Renderer* t_renderer,
                                    StaticPipeline* stapip) {
  if (isLoaded()) {
    if (verticesWithTransparency.empty()) return;

    StaPipTextureBag textureBag;
    StaPipInfoBag infoBag;
    StaPipColorBag colorBag;
    StaPipBag bag;

    textureBag.coordinates = uvMapWithTransparency.data();
    textureBag.texture = BlockManager::getInstance()->getBlocksTexture();

    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
    infoBag.blendingEnabled = true;
    infoBag.antiAliasingEnabled = false;
    infoBag.fullClipChecks = false;
    infoBag.frustumCulling =
        Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

    colorBag.many = verticesColorsWithTransparency.data();

    bag.count = verticesWithTransparency.size();
    bag.vertices = verticesWithTransparency.data();
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    t_renderer->renderer3D.usePipeline(stapip);

    M4x4 rawMatrix = M4x4::Identity;
    infoBag.model = &rawMatrix;

    // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));

    const float distance =
        scaledCenterOffset.distanceTo(camPositon) / CHUNK_DISTANCE;
    if (distance <= 1.5f) {
      return ClippingManager_ClipAndRenderBag(&bag, stapip, t_renderer,
                                              camPositon);
    }

    stapip->core.render(&bag);
  }
};

void Chunk::clear() {
  clearDrawData();
  resetLoadingOffset();

  this->state = ChunkState::Clean;
}

void Chunk::clearDrawData() {
  vertices.clear();
  vertices.shrink_to_fit();
  verticesColors.clear();
  verticesColors.shrink_to_fit();
  uvMap.clear();
  uvMap.shrink_to_fit();

  verticesWithTransparency.clear();
  verticesWithTransparency.shrink_to_fit();
  verticesColorsWithTransparency.clear();
  verticesColorsWithTransparency.shrink_to_fit();
  uvMapWithTransparency.clear();
  uvMapWithTransparency.shrink_to_fit();
}

void Chunk::clearDrawDataWithoutShrink() {
  vertices.clear();
  verticesColors.clear();
  uvMap.clear();

  verticesWithTransparency.clear();
  verticesColorsWithTransparency.clear();
  uvMapWithTransparency.clear();

  // _isDrawDataLoaded = false;
}

void Chunk::build() {
#ifdef DEBUG_MODE
  size_t initialMemoryUsage = get_used_memory();
#endif  // end if DEBUG_MODE

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

          // Build the block mesh
          // TODO: Add AABB data to AABBTree
          // BBox* rawBBox = VertexBlockData::getTorchRawBBox();
          // Vec4 min, max;
          // rawBBox->getMinMax(&min, &max);

          // M4x4 model = ModelBuilder_BuildModel(&offset);
          // min = model * min;
          // max = model * max;

          // // Add data to AABBTree
          // bvh::AABB blockAABB = bvh::AABB();
          // blockAABB.minx = block->minCorner.x;
          // blockAABB.miny = block->minCorner.y;
          // blockAABB.minz = block->minCorner.z;
          // blockAABB.maxx = block->maxCorner.x;
          // blockAABB.maxy = block->maxCorner.y;
          // blockAABB.maxz = block->maxCorner.z;
          // block->tree_index = g_AABBTree->insert(blockAABB, block);

          Block* pBlockTemplate =
              BlockManager::getInstance()->getBlockTemplateByType(block_type);

          if (pBlockTemplate->hasTransparency()) {
            MeshBuilder_BuildMesh(
                &offset, visibleFaces, &verticesWithTransparency,
                &verticesColorsWithTransparency, &uvMapWithTransparency,
                t_worldLightModel, pLevel);
          } else {
            MeshBuilder_BuildMesh(&offset, visibleFaces, &vertices,
                                  &verticesColors, &uvMap, t_worldLightModel,
                                  pLevel);
          }
        }
      }
    }
  }

  state = ChunkState::Loaded;

#ifdef DEBUG_MODE
  size_t totalVertices = vertices.size() + verticesWithTransparency.size();
  if (totalVertices > 0) {
    size_t finalMemoryUsage = get_used_memory();
    float memoryUsage =
        static_cast<float>(finalMemoryUsage - initialMemoryUsage) / 1024.0f;
    printf("Chunk %d memory usage: %.2f KB (Total of vertices: %d)\n", id,
           memoryUsage, totalVertices);
  }
#endif  // end if DEBUG_MODE
}

void Chunk::rebuild() {
  clearDrawData();
  build();
}

void Chunk::reloadLightData() {
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

          if (pBlockTemplate->hasTransparency()) {
            MeshBuilder_BuildLightData(const_cast<Vec4*>(&offset), visibleFaces,
                                       &verticesColorsWithTransparency,
                                       t_worldLightModel, pLevel);
          } else {
            MeshBuilder_BuildLightData(const_cast<Vec4*>(&offset), visibleFaces,
                                       &verticesColors, t_worldLightModel,
                                       pLevel);
          }
        }
      }
    }
  }
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
