#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>

Chunck::Chunck(const Vec4& minCorner, const Vec4& maxCorner, u16 id) {
  this->id = id;
  this->minCorner->set(minCorner);
  this->maxCorner->set(maxCorner);
  this->center->set(((maxCorner - minCorner) / 2 + minCorner));

  u32 count = 8;
  Vec4 vertices[count] = {
      Vec4(minCorner) * DUBLE_BLOCK_SIZE,
      Vec4(maxCorner.x, minCorner.y, minCorner.z) * DUBLE_BLOCK_SIZE,
      Vec4(minCorner.x, maxCorner.y, minCorner.z) * DUBLE_BLOCK_SIZE,
      Vec4(minCorner.x, minCorner.y, maxCorner.z) * DUBLE_BLOCK_SIZE,
      Vec4(maxCorner) * DUBLE_BLOCK_SIZE,
      Vec4(minCorner.x, maxCorner.y, maxCorner.z) * DUBLE_BLOCK_SIZE,
      Vec4(maxCorner.x, minCorner.y, maxCorner.z) * DUBLE_BLOCK_SIZE,
      Vec4(maxCorner.x, maxCorner.y, minCorner.z) * DUBLE_BLOCK_SIZE,
  };
  this->bbox = new BBox(vertices, count);
};

Chunck::~Chunck() {
  this->clear();
  delete this->minCorner;
  delete this->maxCorner;
  delete this->center;
  delete this->bbox;
};

void Chunck::update(const Plane* frustumPlanes) {
  this->_isVisible = this->isChunkVisible(frustumPlanes);
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

u8 Chunck::isChunkVisible(const Plane* frustumPlanes) {
  for (u8 y = 0; y < 8; y++) {
    // 4th frustum plane is NEAR frustum plane
    if (frustumPlanes[4].distanceTo(this->bbox->vertices[y]) >= 0.0F) return 1;
  }
  return 0;  // is behind
}

u8 Chunck::isVisible() { return this->_isVisible; }
