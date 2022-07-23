#include "entities/chunck.hpp"

Chunck::Chunck(BlockManager* t_blockManager) {
  this->blockManager = t_blockManager;
  mcPip.init(&engine->renderer.core);
};

Chunck::~Chunck() {}

void Chunck::update(Player* t_player) {
  this->updateBlocks(*t_player->getPosition());
}

void Chunck::applyFOG(Block* t_block, const Vec4& originPosition) {
  t_block->color.a = 255 * this->getVisibityByPosition(
                               originPosition.distanceTo(*t_block->position));
}

void Chunck::highLightTargetBlock(Block* t_block, u8& isTarget) {
  t_block->color.r = isTarget ? 160 : 128;
  t_block->color.g = isTarget ? 160 : 128;
  t_block->color.b = isTarget ? 160 : 128;
}

void Chunck::renderer(Renderer* t_renderer, MinecraftPipeline* mcPip) {
  t_renderer->renderer3D.usePipeline(*mcPip);
  mcPip->render(singleTexBlocks, singleTexBlocks.size(),
                this->blockManager->getBlocksTexture(), false);
  mcPip->render(multTexBlocks, multTexBlocks.size(),
                this->blockManager->getBlocksTexture(), true);
};

/**
 * Calculate the FOG by distance;
 */
float Chunck::getVisibityByPosition(float d) {
  return Utils::FOG_EXP_GRAD(d, 0.007F, 3.0F);
}

void Chunck::clear() {
  // Clear chunck data
  this->singleTexBlocks.clear();
  this->singleTexBlocks.shrink_to_fit();
  this->multTexBlocks.clear();
  this->multTexBlocks.shrink_to_fit();
}

void Chunck::addBlock(Block* t_block) {
  t_block->isSingleTexture ? this->singleTexBlocks.push_back(t_block)
                           : this->multTexBlocks.push_back(t_block);
}

void Chunck::updateBlocks(const Vec4& playerPosition) {
  // Single texture blocks
  for (u16 blockIndex = 0; blockIndex < this->singleTexBlocks.size();
       blockIndex++) {
    this->applyFOG(&this->singleTexBlocks[blockIndex], playerPosition);
    this->highLightTargetBlock(&this->singleTexBlocks[blockIndex],
                               this->singleTexBlocks[blockIndex]->isTarget);
  }
  // Muilt texture blocks
  for (u16 blockIndex = 0; blockIndex < this->multTexBlocks.size();
       blockIndex++) {
    this->applyFOG(&this->multTexBlocks[blockIndex], playerPosition);
    this->highLightTargetBlock(&this->multTexBlocks[blockIndex],
                               this->multTexBlocks[blockIndex]->isTarget);
  }
}
