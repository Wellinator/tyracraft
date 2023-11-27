#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include "managers/light_manager.hpp"
#include "managers/mesh_builder.hpp"
#include "managers/block/vertex_block_data.hpp"

Chunck::Chunck(const Vec4& minOffset, const Vec4& maxOffset, const u16& id) {
  this->id = id;
  this->tempLoadingOffset->set(minOffset);
  this->minOffset->set(minOffset);
  this->maxOffset->set(maxOffset);
  this->center->set((maxOffset + minOffset) / 2);

  const Vec4 tempMin = minOffset * DUBLE_BLOCK_SIZE;
  const Vec4 tempMax = maxOffset * DUBLE_BLOCK_SIZE;
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

Chunck::~Chunck() {
  clear();
  delete minOffset;
  delete maxOffset;
  delete center;
  delete bbox;
};

void Chunck::init(LevelMap* t_terrain, WorldLightModel* t_worldLightModel) {
  this->t_terrain = t_terrain;
  this->t_worldLightModel = t_worldLightModel;
}

void Chunck::update(const Plane* frustumPlanes) {
  updateFrustumCheck(frustumPlanes);
}

void Chunck::renderer(Renderer* t_renderer, StaticPipeline* stapip,
                      BlockManager* t_blockManager) {
  if (isDrawDataLoaded()) {
    t_renderer->renderer3D.usePipeline(stapip);

    const u8 isPlayerNear = _distanceFromPlayerInChunks <= 3;
    textureBag.texture = isPlayerNear
                             ? t_blockManager->getBlocksTexture()
                             : t_blockManager->getBlocksTextureLowRes();

    M4x4 rawMatrix = M4x4::Identity;
    infoBag.model = &rawMatrix;

    stapip->core.render(&bag);
    // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));
  }
};

void Chunck::clear() {
  clearDrawData();

  for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++) {
    delete this->blocks[blockIndex];
    this->blocks[blockIndex] = NULL;
  }

  this->blocks.clear();
  this->blocks.shrink_to_fit();
  _isPreAllocated = false;

  resetLoadingOffset();

  this->state = ChunkState::Clean;
}

void Chunck::addBlock(Block* t_block) {
  blocks.emplace_back(t_block);
  visibleFacesCount += t_block->visibleFacesCount;
}

// void Chunck::updateBlocks(const Vec4& playerPosition) {}

void Chunck::clearDrawData() {
  vertices.clear();
  vertices.shrink_to_fit();
  verticesColors.clear();
  verticesColors.shrink_to_fit();
  uvMap.clear();
  uvMap.shrink_to_fit();

  _isDrawDataLoaded = false;
  visibleFacesCount = 0;
  blocksCount = 0;
}

void Chunck::loadDrawData() {
  sortBlockByTransparency();

  vertices.reserve(visibleFacesCount * VertexBlockData::FACES_COUNT);
  verticesColors.reserve(visibleFacesCount * VertexBlockData::FACES_COUNT);
  uvMap.reserve(visibleFacesCount * VertexBlockData::FACES_COUNT);

  for (size_t i = 0; i < blocks.size(); i++) {
    MeshBuilder_GenerateBlockMesh(blocks[i], &vertices, &verticesColors, &uvMap,
                                  t_worldLightModel, t_terrain);
  }

  // Pre-load packet data
  textureBag.coordinates = uvMap.data();

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

  _isDrawDataLoaded = true;
  state = ChunkState::Loaded;
}

void Chunck::reloadLightData() {
  verticesColors.clear();

  for (size_t i = 0; i < blocks.size(); i++) {
    if (blocks[i]->isCrossed) {
      MeshBuilder_loadCroosedLightData(blocks[i], &verticesColors,
                                       t_worldLightModel, t_terrain);
    } else {
      MeshBuilder_loadLightData(blocks[i], &verticesColors, t_worldLightModel,
                                t_terrain);
    }
  }

  colorBag.many = verticesColors.data();
}

// TODO: move to a block builder

void Chunck::updateFrustumCheck(const Plane* frustumPlanes) {
  this->frustumCheck = Utils::FrustumAABBIntersect(
      frustumPlanes, &scaledMinOffset, &scaledMaxOffset);
}

void Chunck::sortBlockByTransparency() {
  std::sort(blocks.begin(), blocks.end(), [](const Block* a, const Block* b) {
    return (u8)a->hasTransparency < (u8)b->hasTransparency;
  });
}

void Chunck::preAllocateMemory() {
  blocks.reserve(CHUNCK_LENGTH);
  _isPreAllocated = true;
}

void Chunck::freeUnusedMemory() { blocks.shrink_to_fit(); }

bool Chunck::isPreAllocated() { return _isPreAllocated; }

Block* Chunck::getBlockByPosition(const Vec4* pos) {
  for (size_t i = 0; i < blocks.size(); i++) {
    const auto bPos = blocks[i]->position;
    if (bPos.x == pos->x && bPos.y == pos->y && bPos.z == pos->z)
      return blocks[i];
  }
  return nullptr;
}

Block* Chunck::getBlockByOffset(const Vec4* offset) {
  for (size_t i = 0; i < blocks.size(); i++) {
    Vec4 _tempBlockOffset;
    GetXYZFromPos(&blocks[i]->offset, &_tempBlockOffset);

    if (_tempBlockOffset.x == offset->x && _tempBlockOffset.y == offset->y &&
        _tempBlockOffset.z == offset->z)
      return blocks[i];
  }
  return nullptr;
}

s8 Chunck::getDistanceFromPlayerInChunks() {
  return this->_distanceFromPlayerInChunks;
}

void Chunck::setDistanceFromPlayerInChunks(const s8 distante) {
  this->_distanceFromPlayerInChunks = distante;
};