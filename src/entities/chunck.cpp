#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include "managers/light_manager.hpp"

Chunck::Chunck(const Vec4& minOffset, const Vec4& maxOffset, const u16& id) {
  this->id = id;
  this->tempLoadingOffset->set(minOffset);
  this->minOffset->set(minOffset);
  this->maxOffset->set(maxOffset);
  this->center->set((maxOffset + minOffset) / 2);

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
  this->clear();
  delete this->minOffset;
  delete this->maxOffset;
  delete this->center;
  delete this->bbox;
};

void Chunck::update(const Plane* frustumPlanes, const Vec4& currentPlayerPos,
                    WorldLightModel* worldLightModel) {
  sunPosition.set(worldLightModel->sunPosition);
  sunLightIntensity = worldLightModel->sunLightIntensity;
  ambientLightIntesity = worldLightModel->ambientLightIntensity;
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

void Chunck::highLightTargetBlock(Block* t_block, u8& isTarget) {
  t_block->color.r = isTarget ? 160 : 116;
  t_block->color.g = isTarget ? 160 : 116;
  t_block->color.b = isTarget ? 160 : 116;
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
    infoBag.shadingType = Tyra::TyraShadingGouraud;
    infoBag.textureMappingType = Tyra::TyraNearest;

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

void Chunck::addBlock(Block* t_block) { this->blocks.push_back(t_block); }

void Chunck::updateBlocks(const Vec4& playerPosition) {
  for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++) {
    this->highLightTargetBlock(this->blocks[blockIndex],
                               this->blocks[blockIndex]->isTarget);
  }
}

void Chunck::clearDrawData() {
  vertices.clear();
  vertices.shrink_to_fit();
  verticesColors.clear();
  verticesColors.shrink_to_fit();
  uvMap.clear();
  uvMap.shrink_to_fit();
  // verticesNormals.clear();
  // verticesNormals.shrink_to_fit();

  _isDrawDataLoaded = false;
}

void Chunck::loadDrawData(LevelMap* terrain) {
  sortBlockByTransparency();

  for (size_t i = 0; i < blocks.size(); i++) {
    loadMeshData(blocks[i]);
    loadUVData(blocks[i]);
    loadLightData(terrain, blocks[i]);
  }

  _isDrawDataLoaded = true;
}

void Chunck::reloadLightData(LevelMap* terrain) {
  verticesColors.clear();
  verticesColors.shrink_to_fit();

  for (size_t i = 0; i < blocks.size(); i++) {
    loadLightData(terrain, blocks[i]);
  }
}

void Chunck::loadMeshData(Block* t_block) {
  const Vec4* rawData = VertexBlockData::getVertexData();
  int vert;

  if (t_block->isTopFaceVisible()) {
    vert = 0;
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
    vertices.push_back(t_block->model * rawData[vert++]);
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

  delete[] rawData;
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
    Color faceColor = baseFaceColor;
    auto faceNeightbors = getFaceNeightbors(terrain, FACE_SIDE::TOP, t_block);

    //   Top face 100% of the base color
    LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP, terrain,
                                   sunLightIntensity);
    loadLightFaceDataWithAO(&faceColor, faceNeightbors);
  }

  if (t_block->isBottomFaceVisible()) {
    Color faceColor = baseFaceColor;
    auto faceNeightbors =
        getFaceNeightbors(terrain, FACE_SIDE::BOTTOM, t_block);

    //   Top face 50% of the base color
    LightManager::IntensifyColor(&baseFaceColor, 0.5F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BOTTOM,
                                   terrain, sunLightIntensity);
    loadLightFaceDataWithAO(&faceColor, faceNeightbors);
  }

  if (t_block->isLeftFaceVisible()) {
    Color faceColor = baseFaceColor;
    auto faceNeightbors = getFaceNeightbors(terrain, FACE_SIDE::LEFT, t_block);

    // X-side faces 60% of the base color
    LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::LEFT,
                                   terrain, sunLightIntensity);
    loadLightFaceDataWithAO(&faceColor, faceNeightbors);
  }

  if (t_block->isRightFaceVisible()) {
    Color faceColor = baseFaceColor;
    auto faceNeightbors = getFaceNeightbors(terrain, FACE_SIDE::RIGHT, t_block);

    // X-side faces 60% of the base color
    LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::RIGHT,
                                   terrain, sunLightIntensity);
    loadLightFaceDataWithAO(&faceColor, faceNeightbors);
  }

  if (t_block->isBackFaceVisible()) {
    Color faceColor = baseFaceColor;
    auto faceNeightbors = getFaceNeightbors(terrain, FACE_SIDE::BACK, t_block);

    // Z-side faces 80% of the base color
    LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BACK,
                                   terrain, sunLightIntensity);
    loadLightFaceDataWithAO(&faceColor, faceNeightbors);
  }

  if (t_block->isFrontFaceVisible()) {
    Color faceColor = baseFaceColor;
    auto faceNeightbors = getFaceNeightbors(terrain, FACE_SIDE::FRONT, t_block);

    // Z-side faces 80% of the base color
    LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::FRONT,
                                   terrain, sunLightIntensity);
    loadLightFaceDataWithAO(&faceColor, faceNeightbors);
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
  return block_type != (u8)Blocks::VOID &&
         block_type != (u8)Blocks::AIR_BLOCK &&
         block_type != (u8)Blocks::WATER_BLOCK &&
         block_type != (u8)Blocks::TOTAL_OF_BLOCKS;
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
  auto faceNeightbors = std::array<u8, 8>();
  auto result = std::array<u8, 8>();

  switch (faceSide) {
    case FACE_SIDE::TOP:
      faceNeightbors[0] = GetBlockFromMap(terrain, pos.x, pos.y + 1, pos.z + 1);
      faceNeightbors[1] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z + 1);
      faceNeightbors[2] = GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z);
      faceNeightbors[3] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z - 1);
      faceNeightbors[4] = GetBlockFromMap(terrain, pos.x, pos.y + 1, pos.z - 1);
      faceNeightbors[5] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z - 1);
      faceNeightbors[6] = GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z);
      faceNeightbors[7] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z + 1);
      break;

    case FACE_SIDE::BOTTOM:
      faceNeightbors[0] = GetBlockFromMap(terrain, pos.x, pos.y - 1, pos.z - 1);
      faceNeightbors[1] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z - 1);
      faceNeightbors[2] = GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z);
      faceNeightbors[3] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z + 1);
      faceNeightbors[4] = GetBlockFromMap(terrain, pos.x, pos.y - 1, pos.z + 1);
      faceNeightbors[5] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z + 1);
      faceNeightbors[6] = GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z);
      faceNeightbors[7] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z - 1);
      break;

    case FACE_SIDE::LEFT:
      faceNeightbors[0] = GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z);
      faceNeightbors[1] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z - 1);
      faceNeightbors[2] = GetBlockFromMap(terrain, pos.x + 1, pos.y, pos.z - 1);
      faceNeightbors[3] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z - 1);
      faceNeightbors[4] = GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z);
      faceNeightbors[5] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z + 1);
      faceNeightbors[6] = GetBlockFromMap(terrain, pos.x + 1, pos.y, pos.z + 1);
      faceNeightbors[7] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z + 1);
      break;

    case FACE_SIDE::RIGHT:
      faceNeightbors[0] = GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z);
      faceNeightbors[1] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z + 1);
      faceNeightbors[2] = GetBlockFromMap(terrain, pos.x - 1, pos.y, pos.z + 1);
      faceNeightbors[3] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z + 1);
      faceNeightbors[4] = GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z);
      faceNeightbors[5] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z - 1);
      faceNeightbors[6] = GetBlockFromMap(terrain, pos.x - 1, pos.y, pos.z - 1);
      faceNeightbors[7] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z - 1);
      break;

    case FACE_SIDE::BACK:
      faceNeightbors[0] = GetBlockFromMap(terrain, pos.x, pos.y + 1, pos.z + 1);
      faceNeightbors[1] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z + 1);
      faceNeightbors[2] = GetBlockFromMap(terrain, pos.x + 1, pos.y, pos.z + 1);
      faceNeightbors[3] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z + 1);
      faceNeightbors[4] = GetBlockFromMap(terrain, pos.x, pos.y - 1, pos.z + 1);
      faceNeightbors[5] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z + 1);
      faceNeightbors[6] = GetBlockFromMap(terrain, pos.x - 1, pos.y, pos.z + 1);
      faceNeightbors[7] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z + 1);
      break;

    case FACE_SIDE::FRONT:
      faceNeightbors[0] = GetBlockFromMap(terrain, pos.x, pos.y + 1, pos.z - 1);
      faceNeightbors[1] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y + 1, pos.z - 1);
      faceNeightbors[2] = GetBlockFromMap(terrain, pos.x - 1, pos.y, pos.z - 1);
      faceNeightbors[3] =
          GetBlockFromMap(terrain, pos.x - 1, pos.y - 1, pos.z - 1);
      faceNeightbors[4] = GetBlockFromMap(terrain, pos.x, pos.y - 1, pos.z - 1);
      faceNeightbors[5] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y - 1, pos.z - 1);
      faceNeightbors[6] = GetBlockFromMap(terrain, pos.x + 1, pos.y, pos.z - 1);
      faceNeightbors[7] =
          GetBlockFromMap(terrain, pos.x + 1, pos.y + 1, pos.z - 1);
      break;

    default:
      break;
  }

  result[0] = isBlockOpaque(faceNeightbors[0]);
  result[1] = isBlockOpaque(faceNeightbors[1]);
  result[2] = isBlockOpaque(faceNeightbors[2]);
  result[3] = isBlockOpaque(faceNeightbors[3]);
  result[4] = isBlockOpaque(faceNeightbors[4]);
  result[5] = isBlockOpaque(faceNeightbors[5]);
  result[6] = isBlockOpaque(faceNeightbors[6]);
  result[7] = isBlockOpaque(faceNeightbors[7]);

  return result;
}
