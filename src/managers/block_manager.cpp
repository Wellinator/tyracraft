#include "managers/block_manager.hpp"
#include "file/file_utils.hpp"
#include <math/vec4.hpp>
#include "contants.hpp"

using Tyra::FileUtils;
using Tyra::McpipBlock;
using Tyra::Mesh;
using Tyra::MinecraftPipeline;
using Tyra::Renderer;
using Tyra::Vec4;

BlockManager::BlockManager() {}

BlockManager::~BlockManager() {
  for (u8 i = 0; i < blockItems.size(); i++) {
    delete blockItems[i];
    blockItems[i] = NULL;
  }
  this->blockItems.clear();
}

void BlockManager::init(Renderer* t_renderer, MinecraftPipeline* mcPip) {
  this->t_renderer = t_renderer;
  this->loadBlocksTextures(t_renderer);
  this->registerBlocksTextureCoordinates(mcPip);
  this->registerBlockSoundsEffects();
  this->registerDamageOverlayBlocks(mcPip);
}

void BlockManager::loadBlocksTextures(Renderer* t_renderer) {
  this->blocksTexAtlas = t_renderer->core.texture.repository.add(
      FileUtils::fromCwd("assets/textures/block/texture_atlas.png"));
}

void BlockManager::registerBlocksTextureCoordinates(MinecraftPipeline* mcPip) {
  // Base Blocks
  this->blockItems.push_back(new BlockInfo(GRASS_BLOCK, false,
                                           mcPip->getTextureOffset() * 0,
                                           mcPip->getTextureOffset() * 0));
  this->blockItems.push_back(new BlockInfo(BEDROCK_BLOCK, true,
                                           mcPip->getTextureOffset() * 0,
                                           mcPip->getTextureOffset() * 7));
  this->blockItems.push_back(new BlockInfo(DIRTY_BLOCK, true,
                                           mcPip->getTextureOffset() * 1,
                                           mcPip->getTextureOffset() * 7));
  this->blockItems.push_back(new BlockInfo(SAND_BLOCK, true,
                                           mcPip->getTextureOffset() * 2,
                                           mcPip->getTextureOffset() * 7));
  this->blockItems.push_back(new BlockInfo(STONE_BLOCK, true,
                                           mcPip->getTextureOffset() * 3,
                                           mcPip->getTextureOffset() * 7));
  this->blockItems.push_back(new BlockInfo(WATER_BLOCK, true,
                                           mcPip->getTextureOffset() * 4,
                                           mcPip->getTextureOffset() * 7));
  this->blockItems.push_back(new BlockInfo(GLASS_BLOCK, true,
                                           mcPip->getTextureOffset() * 0,
                                           mcPip->getTextureOffset() * 9));

  // Ores and Minerals
  this->blockItems.push_back(new BlockInfo(GOLD_ORE_BLOCK, true,
                                           mcPip->getTextureOffset() * 0,
                                           mcPip->getTextureOffset() * 8));
  this->blockItems.push_back(new BlockInfo(REDSTONE_ORE_BLOCK, true,
                                           mcPip->getTextureOffset() * 1,
                                           mcPip->getTextureOffset() * 8));
  this->blockItems.push_back(new BlockInfo(IRON_ORE_BLOCK, true,
                                           mcPip->getTextureOffset() * 2,
                                           mcPip->getTextureOffset() * 8));
  this->blockItems.push_back(new BlockInfo(EMERALD_ORE_BLOCK, true,
                                           mcPip->getTextureOffset() * 3,
                                           mcPip->getTextureOffset() * 8));
  this->blockItems.push_back(new BlockInfo(DIAMOND_ORE_BLOCK, true,
                                           mcPip->getTextureOffset() * 4,
                                           mcPip->getTextureOffset() * 8));
  this->blockItems.push_back(new BlockInfo(COAL_ORE_BLOCK, true, 80.0F,
                                           mcPip->getTextureOffset() * 8));

  // Stone bricks
  this->blockItems.push_back(new BlockInfo(STONE_BRICK_BLOCK, true,
                                           mcPip->getTextureOffset() * 1,
                                           mcPip->getTextureOffset() * 6));
  this->blockItems.push_back(new BlockInfo(CRACKED_STONE_BRICKS_BLOCK, true,
                                           mcPip->getTextureOffset() * 0,
                                           mcPip->getTextureOffset() * 6));
  this->blockItems.push_back(new BlockInfo(MOSSY_STONE_BRICKS_BLOCK, true,
                                           mcPip->getTextureOffset() * 2,
                                           mcPip->getTextureOffset() * 6));
  this->blockItems.push_back(new BlockInfo(CHISELED_STONE_BRICKS_BLOCK, true,
                                           mcPip->getTextureOffset() * 3,
                                           mcPip->getTextureOffset() * 6));
  this->blockItems.push_back(new BlockInfo(BRICKS_BLOCK, true,
                                           mcPip->getTextureOffset() * 4,
                                           mcPip->getTextureOffset() * 6));

  // Woods
  this->blockItems.push_back(new BlockInfo(OAK_LOG_BLOCK, false,
                                           mcPip->getTextureOffset() * 1,
                                           mcPip->getTextureOffset() * 0));
  this->blockItems.push_back(new BlockInfo(OAK_LEAVES_BLOCK, true,
                                           mcPip->getTextureOffset() * 3,
                                           mcPip->getTextureOffset() * 10));

  // Stripped Woods
  this->blockItems.push_back(new BlockInfo(STRIPPED_OAK_WOOD_BLOCK, true,
                                           mcPip->getTextureOffset() * 0,
                                           mcPip->getTextureOffset() * 12));

  // Wood Planks
  this->blockItems.push_back(new BlockInfo(OAK_PLANKS_BLOCK, true,
                                           mcPip->getTextureOffset() * 0,
                                           mcPip->getTextureOffset() * 11));
  this->blockItems.push_back(new BlockInfo(SPRUCE_PLANKS_BLOCK, true,
                                           mcPip->getTextureOffset() * 1,
                                           mcPip->getTextureOffset() * 11));
  this->blockItems.push_back(new BlockInfo(ACACIA_PLANKS_BLOCK, true,
                                           mcPip->getTextureOffset() * 2,
                                           mcPip->getTextureOffset() * 11));
  this->blockItems.push_back(new BlockInfo(BIRCH_PLANKS_BLOCK, true,
                                           mcPip->getTextureOffset() * 3,
                                           mcPip->getTextureOffset() * 11));
}

void BlockManager::registerBlockSoundsEffects() {
  this->registerDigSfx();
  this->registerStepSfx();
}

void BlockManager::registerDigSfx() {
  // Base Blocks
  this->digBlockSfx.push_back(
      new SfxBlockModel(GRASS_BLOCK, SoundFxCategory::Dig, SoundFX::Grass1));
  this->digBlockSfx.push_back(
      new SfxBlockModel(BEDROCK_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(
      new SfxBlockModel(DIRTY_BLOCK, SoundFxCategory::Dig, SoundFX::Gravel1));
  this->digBlockSfx.push_back(
      new SfxBlockModel(SAND_BLOCK, SoundFxCategory::Dig, SoundFX::Sand1));
  this->digBlockSfx.push_back(
      new SfxBlockModel(STONE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(
      new SfxBlockModel(GLASS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Ores and Minerals
  this->digBlockSfx.push_back(
      new SfxBlockModel(GOLD_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      REDSTONE_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(
      new SfxBlockModel(IRON_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      EMERALD_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      DIAMOND_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(
      new SfxBlockModel(COAL_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Stone bricks
  this->digBlockSfx.push_back(new SfxBlockModel(
      STONE_BRICK_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      CRACKED_STONE_BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      MOSSY_STONE_BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      CHISELED_STONE_BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->digBlockSfx.push_back(
      new SfxBlockModel(BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Woods
  this->digBlockSfx.push_back(
      new SfxBlockModel(OAK_LOG_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      OAK_LEAVES_BLOCK, SoundFxCategory::Dig, SoundFX::Gravel1));

  // Stripped Woods
  this->digBlockSfx.push_back(new SfxBlockModel(
      STRIPPED_OAK_WOOD_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));

  // Wood Planks
  this->digBlockSfx.push_back(new SfxBlockModel(
      OAK_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      SPRUCE_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      ACACIA_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->digBlockSfx.push_back(new SfxBlockModel(
      BIRCH_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
}

void BlockManager::registerStepSfx() {
  // Base Blocks
  this->stepBlockSfx.push_back(
      new SfxBlockModel(GRASS_BLOCK, SoundFxCategory::Step, SoundFX::Grass1));
  this->stepBlockSfx.push_back(
      new SfxBlockModel(BEDROCK_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(
      new SfxBlockModel(DIRTY_BLOCK, SoundFxCategory::Step, SoundFX::Gravel1));
  this->stepBlockSfx.push_back(
      new SfxBlockModel(SAND_BLOCK, SoundFxCategory::Step, SoundFX::Sand1));
  this->stepBlockSfx.push_back(
      new SfxBlockModel(STONE_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(
      new SfxBlockModel(GLASS_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));

  // Ores and Minerals
  this->stepBlockSfx.push_back(new SfxBlockModel(
      GOLD_ORE_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      REDSTONE_ORE_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      IRON_ORE_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      EMERALD_ORE_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      DIAMOND_ORE_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      COAL_ORE_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));

  // Stone bricks
  this->stepBlockSfx.push_back(new SfxBlockModel(
      STONE_BRICK_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      CRACKED_STONE_BRICKS_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      MOSSY_STONE_BRICKS_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      CHISELED_STONE_BRICKS_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));
  this->stepBlockSfx.push_back(
      new SfxBlockModel(BRICKS_BLOCK, SoundFxCategory::Step, SoundFX::Stone1));

  // Woods
  this->stepBlockSfx.push_back(
      new SfxBlockModel(OAK_LOG_BLOCK, SoundFxCategory::Step, SoundFX::Wood1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      OAK_LEAVES_BLOCK, SoundFxCategory::Step, SoundFX::Gravel1));

  // Stripped Woods
  this->stepBlockSfx.push_back(new SfxBlockModel(
      STRIPPED_OAK_WOOD_BLOCK, SoundFxCategory::Step, SoundFX::Wood1));

  // Wood Planks
  this->stepBlockSfx.push_back(new SfxBlockModel(
      OAK_PLANKS_BLOCK, SoundFxCategory::Step, SoundFX::Wood1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      SPRUCE_PLANKS_BLOCK, SoundFxCategory::Step, SoundFX::Wood1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      ACACIA_PLANKS_BLOCK, SoundFxCategory::Step, SoundFX::Wood1));
  this->stepBlockSfx.push_back(new SfxBlockModel(
      BIRCH_PLANKS_BLOCK, SoundFxCategory::Step, SoundFX::Wood1));
}

BlockInfo* BlockManager::getBlockTexOffsetByType(const u8& blockType) {
  for (u8 i = 0; i < blockItems.size(); i++)
    if (blockItems[i]->blockId == blockType) return blockItems[i];
  TYRA_WARN("Block not found. ID -> ", std::to_string(blockType).c_str(),
            " Was it registered?");
  return nullptr;
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
  return nullptr;
}

SfxBlockModel* BlockManager::getDigSoundByBlockType(const u8& blockType) {
  for (size_t i = 0; i < this->digBlockSfx.size(); i++)
    if (digBlockSfx[i]->getType() == blockType) return digBlockSfx[i];

  TYRA_WARN("Block sound not found for type: ",
            std::to_string(blockType).c_str());
  return nullptr;
}

SfxBlockModel* BlockManager::getStepSoundByBlockType(const u8& blockType) {
  for (size_t i = 0; i < this->stepBlockSfx.size(); i++)
    if (this->stepBlockSfx[i]->getType() == blockType)
      return this->stepBlockSfx[i];

  TYRA_WARN("Block sound not found for type: ",
            std::to_string(blockType).c_str());
  return nullptr;
}