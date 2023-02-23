#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>

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

  loadBags();
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
  sunLightIntensity = worldLightModel->lightIntensity;
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

    M4x4 lightMatrix;
    lightMatrix.identity();
    lightMatrix.scale(10);
    lightMatrix.translate(sunPosition);

    M4x4 rawMatrix;
    rawMatrix.identity();

    PipelineDirLightsBag dirLightsBag;
    dirLightsBag.setAmbientColor(Color(
        ambientLightIntesity, ambientLightIntesity, ambientLightIntesity));
    dirLightsBag.setDirectionalLightColor(
        Color(sunLightIntensity, sunLightIntensity, sunLightIntensity), 0);
    dirLightsBag.setDirectionalLightDirection(
        (sunPosition - CENTER_WORLD_POS).getNormalized(), 0);

    StaPipLightingBag lightBag;
    lightBag.lightMatrix = &lightMatrix;
    lightBag.dirLights = &dirLightsBag;
    lightBag.normals = verticesNormals.data();

    StaPipTextureBag textureBag;
    textureBag.texture = t_blockManager->getBlocksTexture();
    textureBag.coordinates = uvMap.data();

    StaPipInfoBag infoBag;
    infoBag.model = &rawMatrix;
    infoBag.shadingType = Tyra::TyraShadingGouraud;
    infoBag.textureMappingType = Tyra::TyraNearest;

    // Apply multiple colors
    StaPipColorBag colorBag;
    // colorBag.many = verticesColors.data();
    Color baseColor = Color(110.0F, 110.0F, 110.0F);
    colorBag.single = &baseColor;

    StaPipBag bag;
    bag.count = vertices.size();
    bag.vertices = vertices.data();
    bag.lighting = &lightBag;
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
  verticesNormals.clear();
  verticesNormals.shrink_to_fit();

  _isDrawDataLoaded = false;
}

void Chunck::loadDrawData() {
  sortBlockByTransparency();

  const float scale = 1.0F / 16.0F;
  const Vec4 scaleVec = Vec4(scale, scale, 1.0F, 0.0F);
  const Vec4* rawData = VertexBlockData::getVertexData();

  for (size_t i = 0; i < blocks.size(); i++) {
    int vert = 0;

    if (blocks[i]->isTopFaceVisible()) {
      for (size_t j = 0; j < VertexBlockData::FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(120, 120, 120));
        verticesNormals.push_back(Vec4(0.0F, 1.0F, 0.0F));
      }

      const u8& X = blocks[i]->topMapX();
      const u8& Y = blocks[i]->topMapY();

      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
    }
    vert = 6;

    if (blocks[i]->isBottomFaceVisible()) {
      for (size_t j = 0; j < VertexBlockData::FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(60, 60, 60));
        verticesNormals.push_back(Vec4(0.0F, -1.0F, 0.0F));
      }
      const u8& X = blocks[i]->bottomMapX();
      const u8& Y = blocks[i]->bottomMapY();

      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
    }
    vert = 12;

    if (blocks[i]->isLeftFaceVisible()) {
      for (size_t j = 0; j < VertexBlockData::FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(70, 70, 70));
        verticesNormals.push_back(Vec4(0.0F, 0.0F, -1.0F));
      }
      const u8& X = blocks[i]->leftMapX();
      const u8& Y = blocks[i]->leftMapY();

      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);

      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
    }
    vert = 18;

    if (blocks[i]->isRightFaceVisible()) {
      for (size_t j = 0; j < VertexBlockData::FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(100, 100, 100));
        verticesNormals.push_back(Vec4(0.0F, 0.0F, 1.0F));
      }
      const u8& X = blocks[i]->rightMapX();
      const u8& Y = blocks[i]->rightMapY();

      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
    }
    vert = 24;

    if (blocks[i]->isBackFaceVisible()) {
      for (size_t j = 0; j < VertexBlockData::FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(80, 80, 80));
        verticesNormals.push_back(Vec4(1.0F, 0.0F, 0.0F));
      }

      const u8& X = blocks[i]->backMapX();
      const u8& Y = blocks[i]->backMapY();

      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
    }
    vert = 30;

    if (blocks[i]->isFrontFaceVisible()) {
      for (size_t j = 0; j < VertexBlockData::FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(110, 110, 110));
        verticesNormals.push_back(Vec4(-1.0F, 0.0F, 0.0F));
      }
      const u8& X = blocks[i]->frontMapX();
      const u8& Y = blocks[i]->frontMapY();

      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

      uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
      uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
    }
  }

  _isDrawDataLoaded = true;
  delete rawData;
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

void Chunck::loadBags() { return; }

void Chunck::sortBlockByTransparency() {
  std::sort(blocks.begin(), blocks.end(), [](const Block* a, const Block* b) {
    return (u8)a->hasTransparency < (u8)b->hasTransparency;
  });
}
