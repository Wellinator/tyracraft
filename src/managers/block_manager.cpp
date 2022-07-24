#include "managers/block_manager.hpp"
#include "file/file_utils.hpp"

using Tyra::FileUtils;
using Tyra::Mesh;
using Tyra::Renderer;

BlockManager::BlockManager() {}

BlockManager::~BlockManager() {
  for (u8 i = 0; i < blockItems.size(); i++) {
    delete blockItems[i];
    blockItems[i] = NULL;
  }
  this->blockItems.clear();
}

void BlockManager::init(Renderer* t_renderer) {
  this->t_renderer = t_renderer;
  this->loadBlocksTextures();
  this->registerBlocksTextureCoordinates();
}

void BlockManager::loadBlocksTextures() {
  blocksTexAtlas = t_renderer->core.texture.repository.add(
      FileUtils::fromCwd("assets/textures/block/texture_atlas.png"));
}

void BlockManager::registerBlocksTextureCoordinates() {
  // Base Blocks
  this->blockItems.push_back(
      new BlockInfo(GRASS_BLOCK, false, 0.0F, 0.0F));
  this->blockItems.push_back(
      new BlockInfo(BEDROCK_BLOCK, true, 0.0F, 112.0F));
  this->blockItems.push_back(
      new BlockInfo(DIRTY_BLOCK, true, 16.0F, 112.0F));
  this->blockItems.push_back(
      new BlockInfo(SAND_BLOCK, true, 32.0F, 112.0F));
  this->blockItems.push_back(
      new BlockInfo(STONE_BLOCK, true, 48.0F, 112.0F));
  this->blockItems.push_back(
      new BlockInfo(WATER_BLOCK, true, 64.0F, 112.0F));
  this->blockItems.push_back(
      new BlockInfo(GLASS_BLOCK, true, 0.0F, 144.0F));

  // Ores and Minerals
  this->blockItems.push_back(
      new BlockInfo(GOLD_ORE_BLOCK, true, 0.0F, 128.0F));
  this->blockItems.push_back(
      new BlockInfo(REDSTONE_ORE_BLOCK, true, 16.0F, 128.0F));
  this->blockItems.push_back(
      new BlockInfo(IRON_ORE_BLOCK, true, 32.0F, 128.0F));
  this->blockItems.push_back(
      new BlockInfo(EMERALD_ORE_BLOCK, true, 48.0F, 128.0F));
  this->blockItems.push_back(
      new BlockInfo(DIAMOND_ORE_BLOCK, true, 64.0F, 128.0F));
  this->blockItems.push_back(
      new BlockInfo(COAL_ORE_BLOCK, true, 80.0F, 128.0F));

  // Stone bricks
  this->blockItems.push_back(
      new BlockInfo(STONE_BRICK_BLOCK, true, 16.0F, 96.0F));
  this->blockItems.push_back(
      new BlockInfo(CRACKED_STONE_BRICKS_BLOCK, true, 0.0F, 96.0F));
  this->blockItems.push_back(
      new BlockInfo(MOSSY_STONE_BRICKS_BLOCK, true, 32.0F, 96.0F));
  this->blockItems.push_back(
      new BlockInfo(CHISELED_STONE_BRICKS_BLOCK, true, 48.0F, 96.0F));
  this->blockItems.push_back(
      new BlockInfo(BRICKS_BLOCK, true, 64.0F, 96.0F));

  // Woods
  this->blockItems.push_back(
      new BlockInfo(OAK_LOG_BLOCK, false, 10.0F, 0.0F));
  this->blockItems.push_back(
      new BlockInfo(OAK_LEAVES_BLOCK, true, 0.0F, 160.0F));

  // Stripped Woods
  this->blockItems.push_back(
      new BlockInfo(STRIPPED_OAK_WOOD_BLOCK, true, 0.0F, 192.0F));

  // Wood Planks
  this->blockItems.push_back(
      new BlockInfo(OAK_PLANKS_BLOCK, true, 0.0F, 176.0F));
  this->blockItems.push_back(
      new BlockInfo(SPRUCE_PLANKS_BLOCK, true, 16.0F, 176.0F));
  this->blockItems.push_back(
      new BlockInfo(ACACIA_PLANKS_BLOCK, true, 32.0F, 176.0F));
  this->blockItems.push_back(
      new BlockInfo(BIRCH_PLANKS_BLOCK, true, 48.0F, 176.0F));
}

BlockInfo* BlockManager::getBlockTexOffsetByType(u8 blockType) {
  for (int i = 0; i < blockItems.size(); i++)
    if (blockItems[i]->blockId == blockType) return blockItems[i];
  return NULL;
}