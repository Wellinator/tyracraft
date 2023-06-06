#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include "managers/light_manager.hpp"

Chunck::Chunck(const Vec4& minOffset, const Vec4& maxOffset, const u16& id)
    : rawData(VertexBlockData::getVertexData()) {
  this->id = id;
  this->tempLoadingOffset->set(minOffset);
  this->minOffset->set(minOffset);
  this->maxOffset->set(maxOffset);
  this->center->set((maxOffset + minOffset) / 2);

  blocks.reserve(CHUNCK_SIZE);

  // Used to fix the edge of the chunk. It must contain all blocks;
  // const Vec4 offsetFix = Vec4(1.0F);
  const Vec4 tempMin = minOffset * DUBLE_BLOCK_SIZE;  // + offsetFix;
  const Vec4 tempMax = maxOffset * DUBLE_BLOCK_SIZE;  // + offsetFix;

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

void Chunck::update(const Plane* frustumPlanes, const Vec4& currentPlayerPos) {
  this->updateFrustumCheck(frustumPlanes);
  // if (isVisible()) applyFOG(currentPlayerPos);
  // if (!isVisible() && isDrawDataLoaded()) {
  //   clearDrawData();
  // } else if (isVisible() && !isDrawDataLoaded()) {
  //   loadDrawData();
  // }
}

void Chunck::applyFOG(const Vec4& originPosition) {
  // for (size_t i = 0; i < vertices.size(); i++) {
  //   const float interp =
  //       this->getVisibityByPosition(originPosition.distanceTo(vertices[i]));
  // verticesColorsWithFog[i].a = interp * 128;
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

    // bag.lighting = &lightBag;

    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    stapip->core.render(&bag);

    // deallocDrawBags(&bag);
    // t_renderer->renderer3D.utility.drawBBox(*bbox, Color(255, 0, 0));
  }
};

/**
 * Calculate the FOG by distance;
 */
float Chunck::getVisibityByPosition(float d) {
  return Utils::FOG_EXP_GRAD(d, 0.0018F, 3.2F);
}

void Chunck::clear() {
  clearDrawData();

  for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++) {
    delete this->blocks[blockIndex];
    this->blocks[blockIndex] = NULL;
  }

  this->blocks.clear();
  this->blocks.shrink_to_fit();

  resetLoadingOffset();

  this->state = ChunkState::Clean;
}

void Chunck::addBlock(Block* t_block) {
  blocks.push_back(t_block);
  visibleFacesCount += t_block->visibleFacesCount;
}

void Chunck::updateBlocks(const Vec4& playerPosition) {}

void Chunck::clearDrawData() {
  vertices.clear();
  vertices.shrink_to_fit();
  verticesColors.clear();
  verticesColors.shrink_to_fit();
  uvMap.clear();
  uvMap.shrink_to_fit();

  _isDrawDataLoaded = false;
  visibleFacesCount = 0;
}

void Chunck::loadDrawData(LevelMap* terrain,
                          WorldLightModel* t_worldLightModel) {
  sortBlockByTransparency();

  vertices.reserve(visibleFacesCount * VertexBlockData::FACES_COUNT);
  verticesColors.reserve(visibleFacesCount * VertexBlockData::FACES_COUNT);
  uvMap.reserve(visibleFacesCount * VertexBlockData::FACES_COUNT);

  sunPosition.set(t_worldLightModel->sunPosition);
  sunLightIntensity = t_worldLightModel->sunLightIntensity;
  ambientLightIntesity = t_worldLightModel->ambientLightIntensity;

  for (size_t i = 0; i < blocks.size(); i++) {
    loadMeshData(blocks[i]);
    loadUVData(blocks[i]);
    loadLightData(terrain, blocks[i]);
  }

  _isDrawDataLoaded = true;
  state = ChunkState::Loaded;
}

void Chunck::reloadLightData(LevelMap* terrain,
                             WorldLightModel* t_worldLightModel) {
  verticesColors.clear();

  sunPosition.set(t_worldLightModel->sunPosition);
  sunLightIntensity = t_worldLightModel->sunLightIntensity;
  ambientLightIntesity = t_worldLightModel->ambientLightIntensity;

  for (size_t i = 0; i < blocks.size(); i++) {
    loadLightData(terrain, blocks[i]);
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
    if (t_block->type == Blocks::WATER_BLOCK) {
      v0.y *= 0.75F;
      v1.y *= 0.75F;
      v2.y *= 0.75F;
      v3.y *= 0.75F;
      v4.y *= 0.75F;
      v5.y *= 0.75F;
    }

    vertices.push_back(t_block->model * v0);
    vertices.push_back(t_block->model * v1);
    vertices.push_back(t_block->model * v2);
    vertices.push_back(t_block->model * v3);
    vertices.push_back(t_block->model * v4);
    vertices.push_back(t_block->model * v5);
  }
  if (t_block->isBottomFaceVisible()) {
    vert = 6;
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isLeftFaceVisible()) {
    vert = 12;
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isRightFaceVisible()) {
    vert = 18;
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isBackFaceVisible()) {
    vert = 24;
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isFrontFaceVisible()) {
    vert = 30;
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
  }
}

void Chunck::loadUVData(Block* t_block) {
  if (t_block->isTopFaceVisible()) {
    loadUVFaceData(t_block->topMapX(), t_block->topMapY());
  }
  if (t_block->isBottomFaceVisible()) {
    loadUVFaceData(t_block->bottomMapX(), t_block->bottomMapY());
  }
  if (t_block->isLeftFaceVisible()) {
    loadUVFaceData(t_block->leftMapX(), t_block->leftMapY());
  }
  if (t_block->isRightFaceVisible()) {
    loadUVFaceData(t_block->rightMapX(), t_block->rightMapY());
  }
  if (t_block->isBackFaceVisible()) {
    loadUVFaceData(t_block->backMapX(), t_block->backMapY());
  }
  if (t_block->isFrontFaceVisible()) {
    loadUVFaceData(t_block->frontMapX(), t_block->frontMapY());
  }
}

void Chunck::loadUVFaceData(const u8& X, const u8& Y) {
  const float scale = 1.0F / 16.0F;
  const Vec4 scaleVec = Vec4(scale, scale, 1.0F, 0.0F);

  uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

  uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
}

void Chunck::loadLightData(LevelMap* terrain, Block* t_block) {
  auto baseFaceColor = Color(120, 120, 120);

  if (t_block->isTopFaceVisible()) {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP, terrain,
                                   sunLightIntensity);

    if (t_block->type == Blocks::WATER_BLOCK) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors = getFaceNeightbors(terrain, FACE_SIDE::TOP, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isBottomFaceVisible()) {
    //   Top face 50% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.5F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BOTTOM,
                                   terrain, sunLightIntensity);

    if (t_block->type == Blocks::WATER_BLOCK) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors =
          getFaceNeightbors(terrain, FACE_SIDE::BOTTOM, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isLeftFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::LEFT,
                                   terrain, sunLightIntensity);

    if (t_block->type == Blocks::WATER_BLOCK) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors =
          getFaceNeightbors(terrain, FACE_SIDE::LEFT, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isRightFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::RIGHT,
                                   terrain, sunLightIntensity);

    if (t_block->type == Blocks::WATER_BLOCK) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors =
          getFaceNeightbors(terrain, FACE_SIDE::RIGHT, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isBackFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BACK,
                                   terrain, sunLightIntensity);

    if (t_block->type == Blocks::WATER_BLOCK) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors =
          getFaceNeightbors(terrain, FACE_SIDE::BACK, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }

  if (t_block->isFrontFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::FRONT,
                                   terrain, sunLightIntensity);

    if (t_block->type == Blocks::WATER_BLOCK) {
      loadLightFaceData(&faceColor);
    } else {
      auto faceNeightbors =
          getFaceNeightbors(terrain, FACE_SIDE::FRONT, t_block);
      loadLightFaceDataWithAO(&faceColor, faceNeightbors);
    }
  }
}

void Chunck::loadLightFaceDataWithAO(Color* faceColor,
                                     std::array<u8, 8>& faceNeightbors) {
  std::array<u8, 4> AOCornersValues =
      LightManager::getCornersAOValues(faceNeightbors);

  // DEBUG Vertices Colors
  // verticesColors.push_back(Color(255, 0, 0));
  // verticesColors.push_back(Color(0, 255, 0));
  // verticesColors.push_back(Color(0, 0, 255));
  // verticesColors.push_back(Color(255, 255, 0));
  // verticesColors.push_back(Color(0, 255, 255));
  // verticesColors.push_back(Color(255, 0, 255));

  verticesColors.push_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[0])));
  verticesColors.push_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[3])));
  verticesColors.push_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[1])));
  verticesColors.push_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[0])));
  verticesColors.push_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[2])));
  verticesColors.push_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[3])));
}

void Chunck::loadLightFaceData(Color* faceColor) {
  verticesColors.push_back(*faceColor);
  verticesColors.push_back(*faceColor);
  verticesColors.push_back(*faceColor);
  verticesColors.push_back(*faceColor);
  verticesColors.push_back(*faceColor);
  verticesColors.push_back(*faceColor);
}

void Chunck::updateFrustumCheck(const Plane* frustumPlanes) {
  this->frustumCheck = Utils::FrustumAABBIntersect(
      frustumPlanes, *this->minOffset * DUBLE_BLOCK_SIZE,
      *this->maxOffset * DUBLE_BLOCK_SIZE);
}

void Chunck::deallocDrawBags(StaPipBag* bag) {
  if (bag->texture) {
    delete bag->texture;
  }

  if (bag->lighting) {
    delete bag->lighting;
  }
}

void Chunck::sortBlockByTransparency() {
  std::sort(blocks.begin(), blocks.end(), [](const Block* a, const Block* b) {
    return (u8)a->hasTransparency < (u8)b->hasTransparency;
  });
}

bool Chunck::isBlockOpaque(u8 block_type) {
  return block_type != (u8)Blocks::AIR_BLOCK &&
         block_type != (u8)Blocks::GLASS_BLOCK &&
         block_type != (u8)Blocks::WATER_BLOCK;
}

/**
 * Result order:
 * 7  0  1
 * 6     2
 * 5  4  3
 *
 */
std::array<u8, 8> Chunck::getFaceNeightbors(LevelMap* terrain,
                                            FACE_SIDE faceSide, Block* block) {
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
