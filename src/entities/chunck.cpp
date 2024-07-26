#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include "managers/light_manager.hpp"
#include "managers/mesh/mesh_builder.hpp"
#include "managers/collision_manager.hpp"

Chunck::Chunck(const Vec4& minOffset, const Vec4& maxOffset, const u16& id) {
  this->id = id;
  this->minOffset.set(minOffset);
  this->maxOffset.set(maxOffset);
  this->center.set((maxOffset + minOffset) / 2);
  tempLoadingOffset.set(minOffset);

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
    StaPipTextureBag textureBag;
    StaPipInfoBag infoBag;
    StaPipColorBag colorBag;
    StaPipBag bag;

    textureBag.coordinates = uvMap.data();
    textureBag.texture = t_blockManager->getBlocksTexture();

    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
    infoBag.blendingEnabled = false;
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
  }
};

void Chunck::rendererTransparentData(Renderer* t_renderer,
                                     StaticPipeline* stapip,
                                     BlockManager* t_blockManager) {
  if (isDrawDataLoaded()) {
    StaPipTextureBag textureBag;
    StaPipInfoBag infoBag;
    StaPipColorBag colorBag;
    StaPipBag bag;

    textureBag.coordinates = uvMapWithTransparency.data();
    textureBag.texture = t_blockManager->getBlocksTexture();

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
  }
};

void Chunck::clear() {
  clearDrawData();

  visibleFacesCount = 0;
  blocksCount = 0;

  for (u16 i = 0; i < blocks.size(); i++) {
    g_AABBTree->remove(blocks[i]->tree_index);

    delete blocks[i];
    blocks[i] = nullptr;
  }

  blocks.clear();
  blocks.shrink_to_fit();
  _isPreAllocated = false;

  resetLoadingOffset();

  this->state = ChunkState::Clean;
}

void Chunck::clearAsync() {
  size_t counter = 0;
  for (size_t i = _unloaderBatchCounter; i < blocks.size(); i++) {
    g_AABBTree->remove(blocks[i]->tree_index);
    delete blocks[i];
    blocks[i] = nullptr;

    if (counter >= UNLOAD_CHUNK_BATCH) {
      _unloaderBatchCounter = i + 1;
      return;
    } else {
      counter++;
    }
  }

  clearDrawData();

  visibleFacesCount = 0;
  blocksCount = 0;

  blocks.clear();
  blocks.shrink_to_fit();
  _isPreAllocated = false;

  resetLoadingOffset();
  _unloaderBatchCounter = 0;

  state = ChunkState::Clean;
}

// void Chunck::updateBlocks(const Vec4& playerPosition) {}

void Chunck::clearDrawData() {
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

  _isDrawDataLoaded = false;
  _isMemoryReserved = false;
  _isPreAllocated = false;

  _loaderBatchCounter = 0;
  _unloaderBatchCounter = 0;
}

void Chunck::clearDrawDataWithoutShrink() {
  vertices.clear();
  verticesColors.clear();
  uvMap.clear();

  verticesWithTransparency.clear();
  verticesColorsWithTransparency.clear();
  uvMapWithTransparency.clear();

  _isDrawDataLoaded = false;
}

void Chunck::loadDrawDataWithoutSorting() {
  vertices.reserve(visibleFacesCount);
  verticesColors.reserve(visibleFacesCount);
  uvMap.reserve(visibleFacesCount);

  verticesWithTransparency.reserve(visibleFacesCountWithTransparency);
  verticesColorsWithTransparency.reserve(visibleFacesCountWithTransparency);
  uvMapWithTransparency.reserve(visibleFacesCountWithTransparency);

  for (size_t i = 0; i < blocks.size(); i++) {
    if (blocks[i]->hasTransparency) {
      const auto prevSize = verticesWithTransparency.size();
      blocks[i]->drawDataIndex = prevSize > 0 ? prevSize - 1 : 0;
      MeshBuilder_BuildMesh(
          blocks[i], &verticesWithTransparency, &verticesColorsWithTransparency,
          &uvMapWithTransparency, t_worldLightModel, t_terrain);
      blocks[i]->drawDataLength = verticesWithTransparency.size() - prevSize;

      // printf("Transparent Block %i\n", (int)i);
      // printf("drawDataIndex %i | drawDataLength %i \n\n",
      //        blocks[i]->drawDataIndex, blocks[i]->drawDataLength);

    } else {
      const auto prevSize = vertices.size();
      blocks[i]->drawDataIndex = prevSize > 0 ? prevSize - 1 : 0;
      MeshBuilder_BuildMesh(blocks[i], &vertices, &verticesColors, &uvMap,
                            t_worldLightModel, t_terrain);
      blocks[i]->drawDataLength = vertices.size() - prevSize;

      // printf("Block %i\n", (int)i);
      // printf("drawDataIndex %i | drawDataLength %i \n\n",
      //        blocks[i]->drawDataIndex, blocks[i]->drawDataLength);
    }
  }

  _isDrawDataLoaded = true;
}

void Chunck::loadDrawDataAsync() {
  if (_isMemoryReserved == false) {
    vertices.reserve(visibleFacesCount);
    verticesColors.reserve(visibleFacesCount);
    uvMap.reserve(visibleFacesCount);

    verticesWithTransparency.reserve(visibleFacesCountWithTransparency);
    verticesColorsWithTransparency.reserve(visibleFacesCountWithTransparency);
    uvMapWithTransparency.reserve(visibleFacesCountWithTransparency);

    _isMemoryReserved = true;
  }

  size_t counter = 0;
  for (size_t i = _loaderBatchCounter; i < blocks.size(); i++) {
    if (blocks[i]->hasTransparency) {
      MeshBuilder_BuildMesh(
          blocks[i], &verticesWithTransparency, &verticesColorsWithTransparency,
          &uvMapWithTransparency, t_worldLightModel, t_terrain);
    } else {
      MeshBuilder_BuildMesh(blocks[i], &vertices, &verticesColors, &uvMap,
                            t_worldLightModel, t_terrain);
    }

    if (counter >= LOAD_CHUNK_BATCH) {
      _loaderBatchCounter = i + 1;
      return;
    } else {
      counter++;
    }
  }

  _isDrawDataLoaded = true;
  _loaderBatchCounter = 0;
}

void Chunck::loadDrawData() { loadDrawDataWithoutSorting(); }

void Chunck::reloadLightData() {
  verticesColors.clear();
  verticesColorsWithTransparency.clear();

  for (size_t i = 0; i < blocks.size(); i++) {
    if (blocks[i]->hasTransparency) {
      MeshBuilder_BuildLightData(blocks[i], &verticesColorsWithTransparency,
                                 t_worldLightModel, t_terrain);
    } else {
      MeshBuilder_BuildLightData(blocks[i], &verticesColors, t_worldLightModel,
                                 t_terrain);
    }
  }
}

// TODO: move to a block builder

void Chunck::updateFrustumCheck(const Plane* frustumPlanes) {
  this->frustumCheck = Utils::FrustumAABBIntersect(
      frustumPlanes, &scaledMinOffset, &scaledMaxOffset);
}

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

void Chunck::removeBlock(Block* target) {
  blocks.erase(
      std::remove_if(blocks.begin(), blocks.end(),
                     [target](Block* b) { return b->index == target->index; }),
      blocks.end());
  g_AABBTree->remove(target->tree_index);
  delete target;
}

void Chunck::removeBlockByOffset(u32 offset) {
  Vec4 _offsetVec;
  GetXYZFromPos(&offset, &_offsetVec);
  Block* target = getBlockByOffset(&_offsetVec);

  if (target) removeBlock(target);
}

void Chunck::removeBlockByOffset(Vec4* offset) {
  Block* target = getBlockByOffset(offset);
  if (target) removeBlock(target);
}

void Chunck::removeBlockByLocalIndex(u16 index) {
  TYRA_ASSERT(index < blocks.size(), "Invalid block index!");

  Block* target = blocks[index];
  if (target) removeBlock(target);
}

void Chunck::removeBlockByPosition(Vec4* position) {
  Block* target = getBlockByPosition(position);
  if (target) removeBlock(target);
}

u8 Chunck::containsBlock(Vec4* offset) {
  return offset->x >= minOffset.x && offset->x < maxOffset.x &&
         offset->y >= minOffset.y && offset->y < maxOffset.y &&
         offset->z >= minOffset.z && offset->z < maxOffset.z;
}
