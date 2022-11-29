#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>

Chunck::Chunck(const Vec4& minOffset, const Vec4& maxOffset, u16 id) {
  this->id = id;
  this->minOffset->set(minOffset);
  this->maxOffset->set(maxOffset);
  this->center->set(((maxOffset - minOffset) / 2 + minOffset));

  // Used to fix the edge of the chunk. It must contain all blocks;
  const Vec4 offsetFix = Vec4(0.5F);
  const Vec4 min = minOffset - offsetFix;
  const Vec4 max = maxOffset + offsetFix;

  u32 count = 8;
  Vec4 vertices[count] = {
      Vec4(min) * DUBLE_BLOCK_SIZE,
      Vec4(max.x, min.y, min.z) * DUBLE_BLOCK_SIZE,
      Vec4(min.x, max.y, min.z) * DUBLE_BLOCK_SIZE,
      Vec4(min.x, min.y, max.z) * DUBLE_BLOCK_SIZE,
      Vec4(max) * DUBLE_BLOCK_SIZE,
      Vec4(min.x, max.y, max.z) * DUBLE_BLOCK_SIZE,
      Vec4(max.x, min.y, max.z) * DUBLE_BLOCK_SIZE,
      Vec4(max.x, max.y, min.z) * DUBLE_BLOCK_SIZE,
  };
  this->bbox = new BBox(vertices, count);

  loadBags();
};

Chunck::~Chunck() {
  this->clear();
  delete this->minOffset;
  delete this->maxOffset;
  delete this->center;
  delete this->bbox;
};

void Chunck::update(const Plane* frustumPlanes) {
  this->updateFrustumCheck(frustumPlanes);
}

void Chunck::applyFOG(Block* t_block, const Vec4& originPosition) {
  t_block->color.a =
      255 * this->getVisibityByPosition(
                originPosition.distanceTo(*t_block->getPosition()));
}

void Chunck::highLightTargetBlock(Block* t_block, u8& isTarget) {
  t_block->color.r = isTarget ? 160 : 116;
  t_block->color.g = isTarget ? 160 : 116;
  t_block->color.b = isTarget ? 160 : 116;
}

void Chunck::renderer(Renderer* t_renderer, StaticPipeline* stapip,
                      BlockManager* t_blockManager) {
  if (this->state == ChunkState::Loaded && this->isVisible()) {
    t_renderer->renderer3D.usePipeline(stapip);
    stapip->core.render(getDrawData());
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

  this->state = ChunkState::Clean;
}

void Chunck::addBlock(Block* t_block) { this->blocks.push_back(t_block); }

void Chunck::updateBlocks(const Vec4& playerPosition) {
  for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++) {
    // FIXME: refactor fog to use FPU;
    // this->applyFOG(this->blocks[blockIndex], playerPosition);
    this->highLightTargetBlock(this->blocks[blockIndex],
                               this->blocks[blockIndex]->isTarget);
  }
}

void Chunck::clearDrawData() {
  delete bag->vertices;
  bag->count = 0;
}

void Chunck::updateDrawData() {
  clearDrawData();

  for (u16 i = 0; i < blocks.size(); i++) {
    // const auto& frontFace = blocks[i]->bbox->getFrontFace();
    // const auto& backFace = blocks[i]->bbox->getBackFace();
    // const auto& leftFace = blocks[i]->bbox->getLeftFace();
    // const auto& rightFace = blocks[i]->bbox->getRightFace();
    const auto& topFace = blocks[i]->bbox->getTopFace();

    // const auto& bottomFace = blocks[i]->bbox->getBottomFace();

    // TODO: Count the visible faces

    vertices.push_back(
        Vec4(topFace.minCorner.x, topFace.axisPosition, topFace.minCorner.z));
    vertices.push_back(
        Vec4(topFace.maxCorner.x, topFace.axisPosition, topFace.maxCorner.z));
    vertices.push_back(
        Vec4(topFace.minCorner.x, topFace.axisPosition, topFace.maxCorner.z));
    vertices.push_back(
        Vec4(topFace.minCorner.x, topFace.axisPosition, topFace.minCorner.z));
    vertices.push_back(
        Vec4(topFace.maxCorner.x, topFace.axisPosition, topFace.minCorner.z));
    vertices.push_back(
        Vec4(topFace.maxCorner.x, topFace.axisPosition, topFace.maxCorner.z));
  }

  bag->vertices = &vertices.data()[0];
  bag->count = vertices.size();
}

void Chunck::updateFrustumCheck(const Plane* frustumPlanes) {
  this->frustumCheck = Utils::FrustumAABBIntersect(
      frustumPlanes, *this->minOffset * DUBLE_BLOCK_SIZE,
      *this->maxOffset * DUBLE_BLOCK_SIZE);
}

u8 Chunck::isVisible() {
  return this->frustumCheck != Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM;
}

void Chunck::loadBags() {
  // Load info bag
  infoBag = std::make_unique<StaPipInfoBag>();

  infoBag->model = new M4x4();
  infoBag->model->identity();

  infoBag->shadingType = Tyra::TyraShadingGouraud;
  infoBag->textureMappingType = Tyra::TyraNearest;

  // Load color bag
  colorBag = std::make_unique<StaPipColorBag>();
  colorBag->single = new Color(100, 100, 100);

  bag = std::make_unique<StaPipBag>();
  bag->color = colorBag.get();
  bag->info = infoBag.get();
  bag->vertices = &vertices.data()[0];
  bag->count = vertices.size();
}

StaPipBag* Chunck::getDrawData() { return bag.get(); }
