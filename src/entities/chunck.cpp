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
  this->_isVisible = this->isVisible();
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

void Chunck::renderer(Renderer* t_renderer, MinecraftPipeline* mcPip,
                      BlockManager* t_blockManager) {
  if (this->state == ChunkState::Loaded && this->isVisible()) {
    t_renderer->renderer3D.usePipeline(mcPip);
    mcPip->render(this->singleTexBlocks, t_blockManager->getBlocksTexture(),
                  false);
    mcPip->render(this->multiTexBlocks, t_blockManager->getBlocksTexture(),
                  true);
  }
};

/**
 * Calculate the FOG by distance;
 */
float Chunck::getVisibityByPosition(float d) {
  return Utils::FOG_EXP_GRAD(d, 0.0018F, 3.2F);
}

void Chunck::clear() {
  this->clearMcpipBlocks();

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

void Chunck::updateDrawData() { this->filterSingleAndMultiBlocks(); }

void Chunck::filterSingleAndMultiBlocks() {
  for (u16 i = 0; i < this->blocks.size(); i++) {
    McpipBlock* tempMcpipBlock = new McpipBlock();
    tempMcpipBlock->model = &this->blocks[i]->model;
    tempMcpipBlock->color = &this->blocks[i]->color;
    tempMcpipBlock->textureOffset = &this->blocks[i]->textureOffset;
    if (this->blocks[i]->isSingleTexture) {
      singleTexBlocks.push_back(tempMcpipBlock);
    } else {
      multiTexBlocks.push_back(tempMcpipBlock);
    }
  }
  return;
}

void Chunck::clearMcpipBlocks() {
  for (u16 i = 0; i < this->singleTexBlocks.size(); i++) {
    delete this->singleTexBlocks[i];
    this->singleTexBlocks[i] = NULL;
  }
  for (u16 i = 0; i < this->multiTexBlocks.size(); i++) {
    delete this->multiTexBlocks[i];
    this->multiTexBlocks[i] = NULL;
  }

  this->singleTexBlocks.clear();
  this->singleTexBlocks.shrink_to_fit();
  this->multiTexBlocks.clear();
  this->multiTexBlocks.shrink_to_fit();
}

void Chunck::updateFrustumCheck(const Plane* frustumPlanes) {
  this->frustumCheck = this->bbox->frustumCheck(frustumPlanes);
}

u8 Chunck::isVisible() {
  return this->frustumCheck != Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM;
}
