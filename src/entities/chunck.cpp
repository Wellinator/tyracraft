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
  Vec4 _vertices[count] = {
      Vec4(min) * DUBLE_BLOCK_SIZE,
      Vec4(max.x, min.y, min.z) * DUBLE_BLOCK_SIZE,
      Vec4(min.x, max.y, min.z) * DUBLE_BLOCK_SIZE,
      Vec4(min.x, min.y, max.z) * DUBLE_BLOCK_SIZE,
      Vec4(max) * DUBLE_BLOCK_SIZE,
      Vec4(min.x, max.y, max.z) * DUBLE_BLOCK_SIZE,
      Vec4(max.x, min.y, max.z) * DUBLE_BLOCK_SIZE,
      Vec4(max.x, max.y, min.z) * DUBLE_BLOCK_SIZE,
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

// TODO: initi info and color bag once per chunk loading
// TODO: calc light info once per chunk loading
void Chunck::renderer(Renderer* t_renderer, StaticPipeline* stapip,
                      BlockManager* t_blockManager) {
  if (hasDataToDraw() && this->state == ChunkState::Loaded && isVisible()) {
    t_renderer->renderer3D.usePipeline(stapip);

    // Prepare bag

    // Load info bag
    StaPipInfoBag infoBag;
    infoBag.model = new M4x4();
    infoBag.model->identity();
    infoBag.shadingType = Tyra::TyraShadingGouraud;
    infoBag.textureMappingType = Tyra::TyraNearest;

    // Load color bag
    StaPipColorBag colorBag;
    // Color baseColor = Color(128, 128, 128);
    // colorBag.single = &baseColor;
    colorBag.many = verticesColors.data();

    StaPipBag bag;
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.count = vertices.size();
    bag.vertices = vertices.data();

    stapip->core.render(&bag);

    deallocDrawBags(&bag);
    if (infoBag.model) delete infoBag.model;
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
    this->applyFOG(this->blocks[blockIndex], playerPosition);
    this->highLightTargetBlock(this->blocks[blockIndex],
                               this->blocks[blockIndex]->isTarget);
  }
}

void Chunck::clearDrawData() {
  vertices.clear();
  vertices.shrink_to_fit();
  verticesColors.clear();
  verticesColors.shrink_to_fit();
}

void Chunck::updateDrawData() {
  const Vec4* rawData = vertexBlockData.getVertexData();
  for (size_t i = 0; i < blocks.size(); i++) {
    int vert = 0;

    if (blocks[i]->isTopFaceVisible()) {
      for (size_t j = 0; j < vertexBlockData.FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(128, 0, 0));
      }
    }
    vert = 6;

    if (blocks[i]->isBottomFaceVisible()) {
      for (size_t j = 0; j < vertexBlockData.FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(0, 128, 0));
      }
    }
    vert = 12;

    if (blocks[i]->isLeftFaceVisible()) {
      for (size_t j = 0; j < vertexBlockData.FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(0, 0, 128));
      }
    }
    vert = 18;

    if (blocks[i]->isRightFaceVisible()) {
      for (size_t j = 0; j < vertexBlockData.FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(128, 128, 0));
      }
    }
    vert = 24;

    if (blocks[i]->isBackFaceVisible()) {
      for (size_t j = 0; j < vertexBlockData.FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(0, 128, 128));
      }
    }
    vert = 30;

    if (blocks[i]->isFrontFaceVisible()) {
      for (size_t j = 0; j < vertexBlockData.FACES_COUNT; j++) {
        vertices.push_back(blocks[i]->model * rawData[vert++]);
        // verticesColors.push_back(Color(128, 0, 128));
      }
    }
  }

  TYRA_LOG("Loaded ", vertices.size(), " in the chunk ", id);
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
