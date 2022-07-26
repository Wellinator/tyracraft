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
  // this->singleTexBlocks[0].model.print();
  // printf("singleTexBlocks: %i\n", singleTexBlocks.size());
  // printf("multiTexBlocks: %i\n", multiTexBlocks.size());
  mcPip->render(this->singleTexBlocks.data(), singleTexBlocks.size(),
                this->blockManager->getBlocksTexture(), false);
  // mcPip->render(this->multiTexBlocks.data(), multiTexBlocks.size(),
  //               this->blockManager->getBlocksTexture(), true);
};

/**
 * Calculate the FOG by distance;
 */
float Chunck::getVisibityByPosition(float d) {
  return Utils::FOG_EXP_GRAD(d, 0.007F, 3.0F);
}

void Chunck::clear() {
  this->singleTexBlocks.clear();
  this->singleTexBlocks.shrink_to_fit();
  this->multiTexBlocks.clear();
  this->multiTexBlocks.shrink_to_fit();
  this->blocks.clear();
  this->blocks.shrink_to_fit();
}

void Chunck::addBlock(Block* t_block) {
  this->blocks.push_back(*t_block);
  this->setToChanged();
}

void Chunck::updateBlocks(const Vec4& playerPosition) {
  for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++) {
    this->applyFOG(&this->blocks[blockIndex], playerPosition);
    this->highLightTargetBlock(&this->blocks[blockIndex],
                               this->blocks[blockIndex].isTarget);
  }
}

void Chunck::setToChanged() { this->hasChanged = 1; }

void Chunck::filterSingleAndMultiBlocks() {
  std::copy_if(this->blocks.begin(), this->blocks.end(),
               std::back_inserter(this->singleTexBlocks),
               [](Block b) { return b.isSingleTexture; });
  std::copy_if(this->blocks.begin(), this->blocks.end(),
               std::back_inserter(this->multiTexBlocks),
               [](Block b) { return !b.isSingleTexture; });
  this->hasChanged = 0;
  return;
}
