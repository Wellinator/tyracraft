#include "entities/chunck.hpp"
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>

Chunck::Chunck(BlockManager* t_blockManager) {
  this->blockManager = t_blockManager;
};

Chunck::~Chunck() {}

void Chunck::update(Player* t_player) {
  if (this->hasChanged) {
    this->filterSingleAndMultiBlocks();
  }
  this->updateBlocks(*t_player->getPosition());
}

void Chunck::applyFOG(Block* t_block, const Vec4& originPosition) {
  t_block->color.a =
      255 * this->getVisibityByPosition(
                originPosition.distanceTo(*t_block->getPosition()));
}

void Chunck::highLightTargetBlock(Block* t_block, u8& isTarget) {
  t_block->color.r = isTarget ? 160 : 128;
  t_block->color.g = isTarget ? 160 : 128;
  t_block->color.b = isTarget ? 160 : 128;
}

void Chunck::renderer(Renderer* t_renderer, MinecraftPipeline* mcPip) {
  t_renderer->renderer3D.usePipeline(mcPip);
  mcPip->render(this->singleTexBlocks, this->blockManager->getBlocksTexture(),
                false);
  mcPip->render(this->multiTexBlocks, this->blockManager->getBlocksTexture(),
                true);
};

/**
 * Calculate the FOG by distance;
 */
float Chunck::getVisibityByPosition(float d) {
  return Utils::FOG_EXP_GRAD(d, 0.007F, 3.0F);
}

void Chunck::clear() {
  this->clearMcpipBlocks();

  // Delete pointers
  for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++) {
    if (this->blocks[blockIndex] != NULL &&
        this->blocks[blockIndex] != nullptr) {
      delete this->blocks[blockIndex];
      this->blocks[blockIndex] = NULL;
    }
  }

  this->blocks.clear();
  this->blocks.shrink_to_fit();
}

void Chunck::addBlock(Block* t_block) {
  this->blocks.push_back(t_block);
  this->setToChanged();
}

void Chunck::updateBlocks(const Vec4& playerPosition) {
  for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++) {
    // this->applyFOG(this->blocks[blockIndex], playerPosition);
    this->highLightTargetBlock(this->blocks[blockIndex],
                               this->blocks[blockIndex]->isTarget);
  }
}

void Chunck::setToChanged() { this->hasChanged = 1; }

void Chunck::filterSingleAndMultiBlocks() {
  this->clearMcpipBlocks();

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

  this->hasChanged = 0;
  printf("singleTexBlocks: %i\n", singleTexBlocks.size());
  printf("multiTexBlocks: %i\n", multiTexBlocks.size());
  return;
}

void Chunck::clearMcpipBlocks() {
  for (u16 i = 0; i < this->singleTexBlocks.size(); i++) {
    if (this->singleTexBlocks[i] != NULL &&
        this->singleTexBlocks[i] != nullptr) {
      delete this->singleTexBlocks[i];
      this->singleTexBlocks[i] = NULL;
    }
  }

  this->singleTexBlocks.clear();
  this->singleTexBlocks.shrink_to_fit();

  for (u16 i = 0; i < this->multiTexBlocks.size(); i++) {
    if (this->multiTexBlocks[i] != NULL && this->multiTexBlocks[i] != nullptr) {
      delete this->multiTexBlocks[i];
      this->multiTexBlocks[i] = NULL;
    }
  }

  this->multiTexBlocks.clear();
  this->multiTexBlocks.shrink_to_fit();
}
