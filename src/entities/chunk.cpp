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

  if (frustumCheck == Tyra::PARTIALLY_IN_FRUSTUM &&
      _distanceFromPlayerInChunks > -1 && _distanceFromPlayerInChunks < 2) {
    updateSurroundingBlocks();
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

void Chunk::updateSurroundingBlocks() {
  surroundingBlocks.clear();
  surroundingTransparentBlocks.clear();

  u8 offsetRange = 2;
  Vec4 tempOffset = camPositon / DOUBLE_BLOCK_SIZE;
  Vec4 camOffset = Vec4(std::lrint(tempOffset.x), std::lrint(tempOffset.y),
                        std::lrint(tempOffset.z));

  for (s8 _x = -offsetRange; _x <= offsetRange; _x++) {
    for (s8 _y = -offsetRange; _y <= offsetRange; _y++) {
      for (s8 _z = -offsetRange; _z <= offsetRange; _z++) {
        Vec4 offset = camOffset + Vec4(_x, _y, _z);
        u32 blockIndex = getIndexByOffset(offset.x, offset.y, offset.z);

        // Is the block in this chunk?
        if (containsBlock(&offset)) {
          // TODO: render surroungding blocks with clipping algorithm

          // if (Utils::FrustumAABBIntersect(frustumPlanes, &t_block->minCorner,
          //                                 &t_block->maxCorner) ==
          //     CoreBBoxFrustum::PARTIALLY_IN_FRUSTUM) {
          //   if (t_block->hasTransparency()) {
          //     surroundingTransparentBlocks.push_back(t_block);
          //   } else {
          //     surroundingBlocks.push_back(t_block);
          //   }
          // }
        }
      }
    }
  }
}

void Chunk::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  if (isLoaded()) {
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

    stapip->core.render(&bag);
    // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));

    if (surroundingBlocks.size() > 0) {
      renderSolidPartialBlocks(t_renderer, stapip);
    }
  }
};

void Chunk::rendererTransparentData(Renderer* t_renderer,
                                    StaticPipeline* stapip) {
  if (isLoaded()) {
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

    stapip->core.render(&bag);
    // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));

    if (surroundingTransparentBlocks.size() > 0) {
      renderTransparentPartialBlocks(t_renderer, stapip);
    }
  }
};

void Chunk::renderSolidPartialBlocks(Renderer* t_renderer,
                                     StaticPipeline* stapip) {
  for (size_t i = 0; i < surroundingBlocks.size(); i++) {
    Block* t_block = surroundingBlocks[i];
    const auto start = t_block->packed.drawDataIndex;
    const auto end =
        (t_block->packed.drawDataIndex + t_block->packed.drawDataLength);

    //------------------------
    // Temp clipped draw data
    //------------------------
    std::vector<Vec4> inVertices;
    std::vector<Vec4> inUVMap;
    std::vector<Color> inColors;

    inVertices.reserve(t_block->packed.drawDataLength);
    inUVMap.reserve(t_block->packed.drawDataLength);
    inColors.reserve(t_block->packed.drawDataLength);

    std::copy(vertices.begin() + start, vertices.begin() + end,
              std::back_inserter(inVertices));
    std::copy(uvMap.begin() + start, uvMap.begin() + end,
              std::back_inserter(inUVMap));
    std::copy(verticesColors.begin() + start, verticesColors.begin() + end,
              std::back_inserter(inColors));

    // t_renderer->renderer3D.utility.drawBBox(*t_block->bbox, Color(255, 0,
    // 0));
    renderPartialBlockDrawData(t_renderer, false, stapip, inVertices, inUVMap,
                               inColors);
  }
}

void Chunk::renderTransparentPartialBlocks(Renderer* t_renderer,
                                           StaticPipeline* stapip) {
  for (size_t i = 0; i < surroundingTransparentBlocks.size(); i++) {
    Block* t_block = surroundingTransparentBlocks[i];
    const auto start = t_block->packed.drawDataIndex;
    const auto end =
        (t_block->packed.drawDataIndex + t_block->packed.drawDataLength);

    //------------------------
    // Temp clipped draw data
    //------------------------
    std::vector<Vec4> inVertices;
    std::vector<Vec4> inUVMap;
    std::vector<Color> inColors;

    inVertices.reserve(t_block->packed.drawDataLength);
    inUVMap.reserve(t_block->packed.drawDataLength);
    inColors.reserve(t_block->packed.drawDataLength);

    std::copy(verticesWithTransparency.begin() + start,
              verticesWithTransparency.begin() + end,
              std::back_inserter(inVertices));
    std::copy(uvMapWithTransparency.begin() + start,
              uvMapWithTransparency.begin() + end, std::back_inserter(inUVMap));
    std::copy(verticesColorsWithTransparency.begin() + start,
              verticesColorsWithTransparency.begin() + end,
              std::back_inserter(inColors));

    // t_renderer->renderer3D.utility.drawBBox(*t_block->bbox, Color(0, 0,
    // 255));
    renderPartialBlockDrawData(t_renderer, true, stapip, inVertices, inUVMap,
                               inColors);
  }
}

void Chunk::renderPartialBlockDrawData(Renderer* t_renderer, u8 hasTransparency,
                                       StaticPipeline* stapip,
                                       std::vector<Vec4>& in_vertex,
                                       std::vector<Vec4>& in_uv,
                                       std::vector<Color>& in_colors) {
  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  std::vector<Vec4> outVertices;
  std::vector<Vec4> outUVMap;
  std::vector<Color> outColors;

  outVertices.reserve(in_vertex.size());
  outUVMap.reserve(in_vertex.size());
  outColors.reserve(in_vertex.size());

  int generatedVertexCounter =
      ClippingManager_ClipMesh(in_vertex, in_uv, in_colors, outVertices,
                               outUVMap, outColors, t_renderer, camPositon);

  if (generatedVertexCounter == 0) return;

  textureBag.coordinates = outUVMap.data();
  textureBag.texture = BlockManager::getInstance()->getBlocksTexture();

  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
  infoBag.blendingEnabled = true;
  infoBag.antiAliasingEnabled = false;

  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  colorBag.many = outColors.data();

  bag.count = outVertices.size();
  bag.vertices = outVertices.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  t_renderer->renderer3D.usePipeline(stapip);

  M4x4 rawMatrix = M4x4::Identity;
  infoBag.model = &rawMatrix;

  stapip->core.render(&bag);
}

void Chunk::clear() {
  clearDrawData();

  surroundingBlocks.clear();
  surroundingBlocks.shrink_to_fit();

  surroundingTransparentBlocks.clear();
  surroundingTransparentBlocks.shrink_to_fit();

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

  surroundingBlocks.clear();
  surroundingTransparentBlocks.clear();
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
          // delete rawBBox;

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
}

void Chunk::rebuild() {
  clearDrawDataWithoutShrink();
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

        if (block_type != Blocks::AIR_BLOCK) {
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
  return offset->x >= minOffset.x && offset->x < maxOffset.x &&
         offset->y >= minOffset.y && offset->y < maxOffset.y &&
         offset->z >= minOffset.z && offset->z < maxOffset.z;
}
