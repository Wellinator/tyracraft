#include "entities/chunk.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include "managers/light_manager.hpp"
#include "managers/mesh/mesh_builder.hpp"
#include "managers/collision_manager.hpp"
#include "managers/clipping_manager.hpp"
#include "managers/particle/particle_manager.hpp"
#include "managers/particle/flame_particle.hpp"
#include "managers/particle/smoke_particle.hpp"
#include "managers/tick_manager.hpp"
#include "managers/block_manager.hpp"

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

  if (isDrawDataLoaded() && frustumCheck == Tyra::PARTIALLY_IN_FRUSTUM &&
      _distanceFromPlayerInChunks > -1 && _distanceFromPlayerInChunks < 3) {
    updateSurroundingBlocks();
  }
}

void Chunk::tick() {
  u8 emitParticles = this->_distanceFromPlayerInChunks <= 3;

  for (size_t i = 0; i < blocks.size(); i++) {
    if (emitParticles) {
      // TODO: Move to method
      if (blocks[i]->getType() == Blocks::TORCH) {
        if (Utils::Probability(0.008F)) {
          // Creates smoke particle
          SmokeParticle* sp = new SmokeParticle(blocks[i]);
          ParticlesManager::EmitParticle(sp);

          // Creates Flame particle
          Particle* currentParticle =
              ParticlesManager::GetParticleById(blocks[i]->index);

          if (currentParticle) {
            currentParticle->renew();
            return;
          }

          // Emit a flame particle
          FlameParticle* p = new FlameParticle(blocks[i]);
          p->id = blocks[i]->index;
          ParticlesManager::EmitParticle(p);
        }
      }
    }
  }
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

        Block* t_block = nullptr;

        // Is the block in this chunk?
        if (containsBlock(&offset)) {
          t_block = getBlockById(blockIndex);
        } else {
          // Check if the block is in boundaries
          const u8 isInWorld = offset.collidesBox(MIN_WORLD_POS, MAX_WORLD_POS);
          if (isInWorld) {
            // Find the chunk neighbor that contains the block
            Chunk* targetChunk = nullptr;

            if (offset.x < minOffset.x - 1)
              targetChunk = rightNeighbor;
            else if (offset.x > maxOffset.x)
              targetChunk = leftNeighbor;
            else if (offset.y < minOffset.y - 1)
              targetChunk = bottomNeighbor;
            else if (offset.y > maxOffset.y)
              targetChunk = topNeighbor;
            else if (offset.z < minOffset.z - 1)
              targetChunk = frontNeighbor;
            else
              targetChunk = backNeighbor;

            if (targetChunk) t_block = targetChunk->getBlockById(blockIndex);
          }
          continue;
        }

        if (!t_block) continue;

        if (Utils::FrustumAABBIntersect(frustumPlanes, &t_block->minCorner,
                                        &t_block->maxCorner) ==
            CoreBBoxFrustum::PARTIALLY_IN_FRUSTUM) {
          if (t_block->hasTransparency()) {
            surroundingTransparentBlocks.push_back(t_block);
          } else {
            surroundingBlocks.push_back(t_block);
          }
        }
      }
    }
  }
}

void Chunk::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  if (isDrawDataLoaded()) {
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
  if (isDrawDataLoaded()) {
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
    const auto start = t_block->drawDataIndex;
    const auto end = (t_block->drawDataIndex + t_block->drawDataLength);

    //------------------------
    // Temp clipped draw data
    //------------------------
    std::vector<Vec4> inVertices;
    std::vector<Vec4> inUVMap;
    std::vector<Color> inColors;

    inVertices.reserve(t_block->drawDataLength);
    inUVMap.reserve(t_block->drawDataLength);
    inColors.reserve(t_block->drawDataLength);

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
    const auto start = t_block->drawDataIndex;
    const auto end = (t_block->drawDataIndex + t_block->drawDataLength);

    //------------------------
    // Temp clipped draw data
    //------------------------
    std::vector<Vec4> inVertices;
    std::vector<Vec4> inUVMap;
    std::vector<Color> inColors;

    inVertices.reserve(t_block->drawDataLength);
    inUVMap.reserve(t_block->drawDataLength);
    inColors.reserve(t_block->drawDataLength);

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

void Chunk::renderPartialBlockDrawData(Renderer* t_renderer,
                                        u8 hasTransparency,
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

  surroundingBlocks.clear();
  surroundingBlocks.shrink_to_fit();

  surroundingTransparentBlocks.clear();
  surroundingTransparentBlocks.shrink_to_fit();

  resetLoadingOffset();

  this->state = ChunkState::Clean;
}

void Chunk::clearAsync() {
  _isPerformingAsyncTask = true;
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
  _isPerformingAsyncTask = false;
}

// void Chunk::updateBlocks(const Vec4& playerPosition) {}

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

  _isDrawDataLoaded = false;
  _isMemoryReserved = false;
  _isPreAllocated = false;

  _loaderBatchCounter = 0;
  _unloaderBatchCounter = 0;
}

void Chunk::clearDrawDataWithoutShrink() {
  vertices.clear();
  verticesColors.clear();
  uvMap.clear();

  verticesWithTransparency.clear();
  verticesColorsWithTransparency.clear();
  uvMapWithTransparency.clear();

  _isDrawDataLoaded = false;
}

void Chunk::loadDrawDataWithoutSorting() {
  if (_isPerformingAsyncTask == true) return;

  vertices.reserve(visibleFacesCount);
  verticesColors.reserve(visibleFacesCount);
  uvMap.reserve(visibleFacesCount);

  verticesWithTransparency.reserve(visibleFacesCountWithTransparency);
  verticesColorsWithTransparency.reserve(visibleFacesCountWithTransparency);
  uvMapWithTransparency.reserve(visibleFacesCountWithTransparency);

  for (size_t i = 0; i < blocks.size(); i++) {
    if (blocks[i]->hasTransparency()) {
      blocks[i]->drawDataIndex = verticesWithTransparency.size();

      MeshBuilder_BuildMesh(blocks[i], &verticesWithTransparency,
                            &verticesColorsWithTransparency,
                            &uvMapWithTransparency, t_worldLightModel, pLevel);

      blocks[i]->drawDataLength = blocks[i]->visibleFacesCount * 6;

      // printf("Transparent Block %i\n", (int)i);
      // printf("drawDataIndex %i | drawDataLength %i \n\n",
      //        blocks[i]->drawDataIndex, blocks[i]->drawDataLength);

    } else {
      blocks[i]->drawDataIndex = vertices.size();

      MeshBuilder_BuildMesh(blocks[i], &vertices, &verticesColors, &uvMap,
                            t_worldLightModel, pLevel);

      blocks[i]->drawDataLength = blocks[i]->visibleFacesCount * 6;

      // printf("Block %i\n", (int)i);
      // printf("drawDataIndex %i | drawDataLength %i \n\n",
      //        blocks[i]->drawDataIndex, blocks[i]->drawDataLength);
    }
  }

  _isDrawDataLoaded = true;
}

void Chunk::loadDrawDataAsync() {
  if (_isMemoryReserved == false) {
    vertices.reserve(visibleFacesCount);
    verticesColors.reserve(visibleFacesCount);
    uvMap.reserve(visibleFacesCount);

    verticesWithTransparency.reserve(visibleFacesCountWithTransparency);
    verticesColorsWithTransparency.reserve(visibleFacesCountWithTransparency);
    uvMapWithTransparency.reserve(visibleFacesCountWithTransparency);

    _isMemoryReserved = true;
  }

  _isPerformingAsyncTask = true;

  size_t counter = 0;
  for (size_t i = _loaderBatchCounter; i < blocks.size(); i++) {
    if (blocks[i]->hasTransparency()) {
      blocks[i]->drawDataIndex = verticesWithTransparency.size();

      MeshBuilder_BuildMesh(blocks[i], &verticesWithTransparency,
                            &verticesColorsWithTransparency,
                            &uvMapWithTransparency, t_worldLightModel, pLevel);

      blocks[i]->drawDataLength = blocks[i]->visibleFacesCount * 6;
    } else {
      blocks[i]->drawDataIndex = vertices.size();

      MeshBuilder_BuildMesh(blocks[i], &vertices, &verticesColors, &uvMap,
                            t_worldLightModel, pLevel);

      blocks[i]->drawDataLength = blocks[i]->visibleFacesCount * 6;
    }

    if (counter >= LOAD_CHUNK_BATCH) {
      _loaderBatchCounter = i + 1;
      return;
    } else {
      counter++;
    }
  }

  _isDrawDataLoaded = true;
  _isPerformingAsyncTask = false;
  _loaderBatchCounter = 0;
}

void Chunk::loadDrawData() { loadDrawDataWithoutSorting(); }

void Chunk::sortTransParentDrawData(const Vec4& cameraPos) {
  std::vector<Vec4>& vert = verticesWithTransparency;
  std::vector<Color>& col = verticesColorsWithTransparency;
  std::vector<Vec4>& uv = uvMapWithTransparency;

  const size_t numTriangles = vert.size() / 3;
  if (vert.size() % 3 != 0) return;  // Error: Invalid data

  std::vector<size_t> indices(numTriangles);
  std::vector<float> distSq(numTriangles);
  const Vec4 scaledCam = cameraPos * 3.0f;  // Precompute scaled camera position

  // Precompute squared distances for each triangle
  for (size_t i = 0; i < numTriangles; ++i) {
    const size_t base = 3 * i;
    const Vec4 sum = vert[base] + vert[base + 1] + vert[base + 2];
    const Vec4 diff = sum - scaledCam;  // Equivalent to centroid comparison
    distSq[i] = diff.dot3(diff);
    indices[i] = i;
  }

  // Sort indices by ascending distance (front-to-back)
  std::sort(indices.begin(), indices.end(),
            [&](size_t a, size_t b) { return distSq[a] > distSq[b]; });

  // Build sorted triangle array
  std::vector<Vec4> sortedVert;
  std::vector<Color> sortedCol;
  std::vector<Vec4> sortedUV;

  sortedVert.reserve(vert.size());
  sortedCol.reserve(vert.size());
  sortedUV.reserve(vert.size());

  for (size_t idx : indices) {
    const size_t base = 3 * idx;
    const size_t p1 = base;
    const size_t p2 = base + 1;
    const size_t p3 = base + 2;

    sortedVert.push_back(vert[p1]);
    sortedVert.push_back(vert[p2]);
    sortedVert.push_back(vert[p3]);

    sortedCol.push_back(col[p1]);
    sortedCol.push_back(col[p2]);
    sortedCol.push_back(col[p3]);

    sortedUV.push_back(uv[p1]);
    sortedUV.push_back(uv[p2]);
    sortedUV.push_back(uv[p3]);
  }

  // Replace original data
  verticesWithTransparency = std::move(sortedVert);
  verticesColorsWithTransparency = std::move(sortedCol);
  uvMapWithTransparency = std::move(sortedUV);
}

void Chunk::reloadLightData() {
  if (_isPerformingAsyncTask == true) return;

  verticesColors.clear();
  verticesColorsWithTransparency.clear();

  for (size_t i = 0; i < blocks.size(); i++) {
    if (blocks[i]->hasTransparency()) {
      MeshBuilder_BuildLightData(blocks[i], &verticesColorsWithTransparency,
                                 t_worldLightModel, pLevel);
    } else {
      MeshBuilder_BuildLightData(blocks[i], &verticesColors, t_worldLightModel,
                                 pLevel);
    }
  }
}

// TODO: move to a block builder

void Chunk::updateFrustumCheck(const Plane* frustumPlanes) {
  this->frustumPlanes = (Plane*)frustumPlanes;
  this->frustumCheck = Utils::FrustumAABBIntersect(
      frustumPlanes, &scaledMinOffset, &scaledMaxOffset);
}

Block* Chunk::getBlockByPosition(const Vec4* pos) {
  for (size_t i = 0; i < blocks.size(); i++) {
    const auto bPos = blocks[i]->position;
    if (bPos.x == pos->x && bPos.y == pos->y && bPos.z == pos->z)
      return blocks[i];
  }
  return nullptr;
}

Block* Chunk::getBlockByOffset(const Vec4* offset) {
  for (size_t i = 0; i < blocks.size(); i++) {
    Vec4 _tempBlockOffset;
    pLevel->GetXYZFromPos(&blocks[i]->offset, &_tempBlockOffset);

    if (_tempBlockOffset.x == offset->x && _tempBlockOffset.y == offset->y &&
        _tempBlockOffset.z == offset->z)
      return blocks[i];
  }
  return nullptr;
}

Block* Chunk::getBlockById(const u32 blockId) {
  for (size_t i = 0; i < blocks.size(); i++) {
    if (blocks[i]->index == blockId) return blocks[i];
  }
  return nullptr;
}

void Chunk::removeBlock(Block* target) {
  blocks.erase(
      std::remove_if(blocks.begin(), blocks.end(),
                     [target](Block* b) { return b->index == target->index; }),
      blocks.end());
  g_AABBTree->remove(target->tree_index);
  delete target;
}

void Chunk::removeBlockByOffset(u32 offset) {
  Vec4 _offsetVec;
  pLevel->GetXYZFromPos(&offset, &_offsetVec);
  Block* target = getBlockByOffset(&_offsetVec);

  if (target) removeBlock(target);
}

void Chunk::removeBlockByOffset(Vec4* offset) {
  Block* target = getBlockByOffset(offset);
  if (target) removeBlock(target);
}

void Chunk::removeBlockByLocalIndex(u16 index) {
  TYRA_ASSERT(index < blocks.size(), "Invalid block index!");

  Block* target = blocks[index];
  if (target) removeBlock(target);
}

void Chunk::removeBlockByPosition(Vec4* position) {
  Block* target = getBlockByPosition(position);
  if (target) removeBlock(target);
}

u8 Chunk::containsBlock(Vec4* offset) {
  return offset->x >= minOffset.x && offset->x < maxOffset.x &&
         offset->y >= minOffset.y && offset->y < maxOffset.y &&
         offset->z >= minOffset.z && offset->z < maxOffset.z;
}
