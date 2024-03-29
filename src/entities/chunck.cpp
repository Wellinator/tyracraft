#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include "managers/light_manager.hpp"

Chunck::Chunck(const Vec4& minOffset, const Vec4& maxOffset, const u16& id)
    : rawData(VertexBlockData::getVertexData()),
      crossBlockRawData(VertexBlockData::getCrossedVertexData()) {
  this->id = id;
  this->tempLoadingOffset->set(minOffset);
  this->minOffset->set(minOffset);
  this->maxOffset->set(maxOffset);
  this->center->set((maxOffset + minOffset) / 2);

  const Vec4 tempMin = minOffset * DUBLE_BLOCK_SIZE;
  const Vec4 tempMax = maxOffset * DUBLE_BLOCK_SIZE;

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

  delete[] rawData;
};

void Chunck::init(LevelMap* terrain, WorldLightModel* t_worldLightModel) {
  this->terrain = terrain;
  this->t_worldLightModel = t_worldLightModel;
}

void Chunck::update(const Plane* frustumPlanes, const Vec4& currentPlayerPos) {
  this->updateFrustumCheck(frustumPlanes);
  // if (!isVisible() && isDrawDataLoaded()) {
  //   clearDrawData();
  // } else if (isVisible() && !isDrawDataLoaded()) {
  //   loadDrawData();
  // }
}

void Chunck::renderer(Renderer* t_renderer, StaticPipeline* stapip,
                      BlockManager* t_blockManager) {
  if (isDrawDataLoaded()) {
    t_renderer->renderer3D.usePipeline(stapip);

    M4x4 rawMatrix;
    rawMatrix.identity();

    StaPipTextureBag textureBag;
    textureBag.texture = t_blockManager->getBlocksTexture();
    textureBag.coordinates = uvMap.data();

    StaPipInfoBag infoBag;
    infoBag.model = &rawMatrix;
    infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.blendingEnabled = true;
    infoBag.antiAliasingEnabled = false;

    // Apply multiple colors
    StaPipColorBag colorBag;
    colorBag.many = verticesColors.data();

    StaPipBag bag;
    bag.count = vertices.size();
    bag.vertices = vertices.data();

    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

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
    if (blocks[i]->isCrossed) {
      loadCrossBlock(blocks[i]);
    } else {
      loadCuboidBlock(blocks[i]);
    }
  }

  _isDrawDataLoaded = true;
  state = ChunkState::Loaded;
}

void Chunck::loadCuboidBlock(Block* t_block) {
  loadMeshData(t_block);
  loadUVData(t_block);
  loadLightData(t_block);
}

void Chunck::loadCrossBlock(Block* t_block) {
  loadCrossedMeshData(t_block);
  loadCrossedUVData(t_block);
  loadCroosedLightData(t_block);
}

void Chunck::reloadLightData() {
  verticesColors.clear();
  for (size_t i = 0; i < blocks.size(); i++) {
    if (blocks[i]->isCrossed) {
      loadCroosedLightData(blocks[i]);
    } else {
      loadLightData(blocks[i]);
    }
  }
}

void Chunck::loadMeshData(Block* t_block) {
  int vert;

  if (t_block->isTopFaceVisible()) {
    vert = 0;
    Vec4 v0 = rawData[vert++];
    Vec4 v1 = rawData[vert++];
    Vec4 v2 = rawData[vert++];
    Vec4 v3 = rawData[vert++];
    Vec4 v4 = rawData[vert++];
    Vec4 v5 = rawData[vert++];

    // TODO: move to a block builder
    if (t_block->type == Blocks::WATER_BLOCK || t_block->isCrossed) {
      v0.y *= 0.75F;
      v1.y *= 0.75F;
      v2.y *= 0.75F;
      v3.y *= 0.75F;
      v4.y *= 0.75F;
      v5.y *= 0.75F;
    }

    vertices.emplace_back(v0 * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(v1 * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(v2 * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(v3 * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(v4 * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(v5 * BLOCK_SIZE + t_block->position);
  }
  if (t_block->isBottomFaceVisible()) {
    vert = 6;
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
  }
  if (t_block->isLeftFaceVisible()) {
    vert = 12;
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
  }
  if (t_block->isRightFaceVisible()) {
    vert = 18;
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
  }
  if (t_block->isBackFaceVisible()) {
    vert = 24;
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
  }
  if (t_block->isFrontFaceVisible()) {
    vert = 30;
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
    vertices.emplace_back(rawData[vert++] * BLOCK_SIZE + t_block->position);
  }
}

void Chunck::loadUVData(Block* t_block) {
  if (t_block->isTopFaceVisible()) {
    loadUVFaceData(t_block->facesMapIndex[0]);
  }
  if (t_block->isBottomFaceVisible()) {
    loadUVFaceData(t_block->facesMapIndex[1]);
  }
  if (t_block->isLeftFaceVisible()) {
    loadUVFaceData(t_block->facesMapIndex[2]);
  }
  if (t_block->isRightFaceVisible()) {
    loadUVFaceData(t_block->facesMapIndex[3]);
  }
  if (t_block->isBackFaceVisible()) {
    loadUVFaceData(t_block->facesMapIndex[4]);
  }
  if (t_block->isFrontFaceVisible()) {
    loadUVFaceData(t_block->facesMapIndex[5]);
  }
}

void Chunck::loadUVFaceData(const u8& index) {
  const u8 X = index < MAX_TEX_COLS ? index : index % MAX_TEX_COLS;
  const u8 Y = index < MAX_TEX_COLS ? 0 : std::floor(index / MAX_TEX_COLS);
  const float scale = 1.0F / 16.0F;
  const Vec4 scaleVec = Vec4(scale, scale, 1.0F, 0.0F);

  uvMap.emplace_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
  uvMap.emplace_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
  uvMap.emplace_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

  uvMap.emplace_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
  uvMap.emplace_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
  uvMap.emplace_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
}

void Chunck::loadLightData(Block* t_block) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  if (t_block->isTopFaceVisible()) {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP, terrain,
                                   t_worldLightModel->sunLightIntensity);

    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    if (t_block->type == Blocks::WATER_BLOCK || t_block->isCrossed) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors = getFaceNeightbors(FACE_SIDE::TOP, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isBottomFaceVisible()) {
    //   Top face 50% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.5F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BOTTOM,
                                   terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    if (t_block->type == Blocks::WATER_BLOCK || t_block->isCrossed) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors = getFaceNeightbors(FACE_SIDE::BOTTOM, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isLeftFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::LEFT,
                                   terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    if (t_block->type == Blocks::WATER_BLOCK || t_block->isCrossed) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors = getFaceNeightbors(FACE_SIDE::LEFT, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isRightFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::RIGHT,
                                   terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    if (t_block->type == Blocks::WATER_BLOCK || t_block->isCrossed) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors = getFaceNeightbors(FACE_SIDE::RIGHT, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isBackFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BACK,
                                   terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    if (t_block->type == Blocks::WATER_BLOCK || t_block->isCrossed) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors = getFaceNeightbors(FACE_SIDE::BACK, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isFrontFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::FRONT,
                                   terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    if (t_block->type == Blocks::WATER_BLOCK || t_block->isCrossed) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors = getFaceNeightbors(FACE_SIDE::FRONT, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  blockColorAverage /= t_block->visibleFacesCount;
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
}

void Chunck::loadCroosedLightData(Block* t_block) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  // Face 1
  {
    Color faceColor = baseFaceColor;
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP, terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    loadLightFaceData(&faceColor);
  }

  // Face 2
  {
    Color faceColor = baseFaceColor;
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP, terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    loadLightFaceData(&faceColor);
  }

  blockColorAverage /= t_block->visibleFacesCount;
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
}

void Chunck::loadCrossedMeshData(Block* t_block) {
  int vert = 0;

  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);

  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
  vertices.emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                        t_block->position);
}

void Chunck::loadCrossedUVData(Block* t_block) {
  loadUVFaceData(t_block->facesMapIndex[0]);
  loadUVFaceData(t_block->facesMapIndex[0]);
}

void Chunck::loadLightFaceDataWithAO(Color* faceColor,
                                     std::array<u8, 8>& faceNeightbors) {
  std::array<u8, 4> AOCornersValues =
      LightManager::getCornersAOValues(faceNeightbors);

  // DEBUG Vertices Colors
  // verticesColors.emplace_back(Color(255, 0, 0));
  // verticesColors.emplace_back(Color(0, 255, 0));
  // verticesColors.emplace_back(Color(0, 0, 255));
  // verticesColors.emplace_back(Color(255, 255, 0));
  // verticesColors.emplace_back(Color(0, 255, 255));
  // verticesColors.emplace_back(Color(255, 0, 255));

  verticesColors.emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[0])));
  verticesColors.emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[3])));
  verticesColors.emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[1])));
  verticesColors.emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[0])));
  verticesColors.emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[2])));
  verticesColors.emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[3])));
}

void Chunck::loadLightFaceData(Color* faceColor) {
  verticesColors.emplace_back(*faceColor);
  verticesColors.emplace_back(*faceColor);
  verticesColors.emplace_back(*faceColor);
  verticesColors.emplace_back(*faceColor);
  verticesColors.emplace_back(*faceColor);
  verticesColors.emplace_back(*faceColor);
}

void Chunck::updateFrustumCheck(const Plane* frustumPlanes) {
  this->frustumCheck = Utils::FrustumAABBIntersect(
      frustumPlanes, *this->minOffset * DUBLE_BLOCK_SIZE,
      *this->maxOffset * DUBLE_BLOCK_SIZE);
}

void Chunck::sortBlockByTransparency() {
  std::sort(blocks.begin(), blocks.end(), [](const Block* a, const Block* b) {
    return (u8)a->hasTransparency < (u8)b->hasTransparency;
  });
}

bool Chunck::isBlockOpaque(u8 block_type) {
  return block_type != (u8)Blocks::AIR_BLOCK &&
         block_type != (u8)Blocks::GLASS_BLOCK &&
         block_type != (u8)Blocks::POPPY_FLOWER &&
         block_type != (u8)Blocks::DANDELION_FLOWER &&
         block_type != (u8)Blocks::GRASS &&
         block_type != (u8)Blocks::WATER_BLOCK;
}

/**
 * Result order:
 * 7  0  1
 * 6     2
 * 5  4  3
 *
 */
std::array<u8, 8> Chunck::getFaceNeightbors(FACE_SIDE faceSide, Block* block) {
  auto pos = block->offset;
  auto result = std::array<u8, 8>();

  switch (faceSide) {
    case FACE_SIDE::TOP:
      result[0] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x, pos.y + 1, pos.z + 1));
      result[1] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      result[2] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z));
      result[3] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      result[4] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x, pos.y + 1, pos.z - 1));
      result[5] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z - 1));
      result[6] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z));
      result[7] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::BOTTOM:
      result[0] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x, pos.y - 1, pos.z - 1));
      result[1] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[2] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z));
      result[3] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[4] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x, pos.y - 1, pos.z + 1));
      result[5] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[6] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z));
      result[7] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      break;

    case FACE_SIDE::LEFT:
      result[0] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z));
      result[1] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z - 1));
      result[2] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x + 1, pos.y, pos.z - 1));
      result[3] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      result[4] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z));
      result[5] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[6] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x + 1, pos.y, pos.z + 1));
      result[7] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::RIGHT:
      result[0] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z));
      result[1] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      result[2] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x - 1, pos.y, pos.z + 1));
      result[3] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[4] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z));
      result[5] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[6] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x - 1, pos.y, pos.z - 1));
      result[7] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      break;

    case FACE_SIDE::BACK:
      result[0] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x, pos.y + 1, pos.z + 1));
      result[1] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      result[2] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x + 1, pos.y, pos.z + 1));
      result[3] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[4] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x, pos.y - 1, pos.z + 1));
      result[5] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[6] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x - 1, pos.y, pos.z + 1));
      result[7] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::FRONT:
      result[0] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x, pos.y + 1, pos.z - 1));
      result[1] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      result[2] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x - 1, pos.y, pos.z - 1));
      result[3] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[4] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x, pos.y - 1, pos.z - 1));
      result[5] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      result[6] =
          isBlockOpaque(GetBlockFromMap(terrain, pos.x + 1, pos.y, pos.z - 1));
      result[7] = isBlockOpaque(
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z - 1));
      break;

    default:
      break;
  }

  return result;
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
    if (blocks[i]->offset.x == offset->x && blocks[i]->offset.y == offset->y &&
        blocks[i]->offset.z == offset->z)
      return blocks[i];
  }
  return nullptr;
}