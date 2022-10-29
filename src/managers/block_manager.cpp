#include "managers/block_manager.hpp"
#include "file/file_utils.hpp"
#include <math/vec4.hpp>
#include "contants.hpp"

using Tyra::FileUtils;
using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::Renderer;
using Tyra::Vec4;

BlockManager::BlockManager() {}

BlockManager::~BlockManager() {
  delete this->t_blockTextureRepository;

  this->t_renderer->getTextureRepository().free(this->blocksTexAtlas->id);

  for (u8 i = 0; i < this->blockSfxRepositories.size(); i++) {
    delete this->blockSfxRepositories[i];
    this->blockSfxRepositories[i] = NULL;
  }
  this->blockSfxRepositories.clear();

  for (u8 i = 0; i < this->damage_overlay.size(); i++) {
    delete this->damage_overlay[i];
    this->damage_overlay[i] = NULL;
  }
  this->damage_overlay.clear();
}

void BlockManager::init(Renderer* t_renderer, MinecraftPipeline* mcPip) {
  this->t_renderer = t_renderer;
  this->t_blockTextureRepository = new BlockTextureRepository(mcPip);
  this->loadBlocksTextures(t_renderer);
  this->registerBlockSoundsEffects();
  this->registerDamageOverlayBlocks(mcPip);
}

void BlockManager::loadBlocksTextures(Renderer* t_renderer) {
  this->blocksTexAtlas = t_renderer->core.texture.repository.add(
      FileUtils::fromCwd("assets/textures/block/texture_atlas.png"));
}

void BlockManager::registerBlockSoundsEffects() {
  this->blockSfxRepositories.push_back(new BlockDigSfxRepository());
  this->blockSfxRepositories.push_back(new BlockStepSfxRepository());
}

BlockInfo* BlockManager::getBlockTexOffsetByType(const u8& blockType) {
  return this->t_blockTextureRepository->getTextureInfo(blockType);
}

float BlockManager::getBlockBreakingTime() {
  // TODO: implement https://minecraft.fandom.com/wiki/Breaking
  return 0.9F;
}

void BlockManager::registerDamageOverlayBlocks(MinecraftPipeline* mcPip) {
  float offsetY = mcPip->getTextureOffset() * 15;
  for (u8 i = 0; i <= 10; i++) {
    McpipBlock* damageOverlay = new McpipBlock();

    damageOverlay->textureOffset =
        new Vec4(mcPip->getTextureOffset() * i, offsetY, 0.0F, 1.0F);
    this->damage_overlay.push_back(damageOverlay);
  }
}

McpipBlock* BlockManager::getDamageOverlay(const float& damage_percentage) {
  int normal_damage = floor(damage_percentage / 10);
  for (u8 i = 0; i < damage_overlay.size(); i++)
    if (i >= normal_damage) return damage_overlay[i];

  TYRA_WARN("Block damage overlay not loaded");
  return nullptr;
}

SfxBlockModel* BlockManager::getDigSoundByBlockType(const u8& blockType) {
  for (size_t i = 0; i < this->blockSfxRepositories.size(); i++)
    if (blockSfxRepositories[i]->id == SoundFxCategory::Dig)
      return blockSfxRepositories[i]->getModel(blockType);

  TYRA_WARN("Block sound not found for type: ",
            std::to_string(blockType).c_str());
  return nullptr;
}

SfxBlockModel* BlockManager::getStepSoundByBlockType(const u8& blockType) {
  for (size_t i = 0; i < this->blockSfxRepositories.size(); i++)
    if (this->blockSfxRepositories[i]->id == SoundFxCategory::Step)
      return this->blockSfxRepositories[i]->getModel(blockType);

  TYRA_WARN("Block sound not found for type: ",
            std::to_string(blockType).c_str());
  return nullptr;
}