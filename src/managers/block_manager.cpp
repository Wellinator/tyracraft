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
      FileUtils::fromCwd("aseets/textures/block/texture_atlas.png"));
}

void BlockManager::registerBlocksTextureCoordinates() {
  // Base Blocks
  this->blockItems.push_back(
      new BlockTexOffset(GRASS_BLOCK, false, 0.0F, 0.0F));
  this->blockItems.push_back(
      new BlockTexOffset(BEDROCK_BLOCK, true, 0.0F, 112.0F));
  this->blockItems.push_back(
      new BlockTexOffset(DIRTY_BLOCK, true, 16.0F, 112.0F));
  this->blockItems.push_back(
      new BlockTexOffset(SAND_BLOCK, true, 32.0F, 112.0F));
  this->blockItems.push_back(
      new BlockTexOffset(STONE_BLOCK, true, 48.0F, 112.0F));
  this->blockItems.push_back(
      new BlockTexOffset(WATER_BLOCK, true, 64.0F, 112.0F));
  this->blockItems.push_back(
      new BlockTexOffset(GLASS_BLOCK, true, 0.0F, 144.0F));

  // Ores and Minerals
  this->blockItems.push_back(
      new BlockTexOffset(GOLD_ORE_BLOCK, true, 0.0F, 128.0F));
  this->blockItems.push_back(
      new BlockTexOffset(REDSTONE_ORE_BLOCK, true, 16.0F, 128.0F));
  this->blockItems.push_back(
      new BlockTexOffset(IRON_ORE_BLOCK, true, 32.0F, 128.0F));
  this->blockItems.push_back(
      new BlockTexOffset(EMERALD_ORE_BLOCK, true, 48.0F, 128.0F));
  this->blockItems.push_back(
      new BlockTexOffset(DIAMOND_ORE_BLOCK, true, 64.0F, 128.0F));
  this->blockItems.push_back(
      new BlockTexOffset(COAL_ORE_BLOCK, true, 80.0F, 128.0F));

  // Stone bricks
  this->blockItems.push_back(
      new BlockTexOffset(STONE_BRICK_BLOCK, true, 16.0F, 96.0F));
  this->blockItems.push_back(
      new BlockTexOffset(CRACKED_STONE_BRICKS_BLOCK, true, 0.0F, 96.0F));
  this->blockItems.push_back(
      new BlockTexOffset(MOSSY_STONE_BRICKS_BLOCK, true, 32.0F, 96.0F));
  this->blockItems.push_back(
      new BlockTexOffset(CHISELED_STONE_BRICKS_BLOCK, true, 48.0F, 96.0F));
  this->blockItems.push_back(
      new BlockTexOffset(BRICKS_BLOCK, true, 64.0F, 96.0F));

  // Woods
  this->blockItems.push_back(
      new BlockTexOffset(OAK_LOG_BLOCK, false, 10.0F, 0.0F));
  this->blockItems.push_back(
      new BlockTexOffset(OAK_LEAVES_BLOCK, true, 0.0F, 160.0F));

  // Stripped Woods
  this->blockItems.push_back(
      new BlockTexOffset(STRIPPED_OAK_WOOD_BLOCK, true, 0.0F, 192.0F));

  // Wood Planks
  this->blockItems.push_back(
      new BlockTexOffset(OAK_PLANKS_BLOCK, true, 0.0F, 176.0F));
  this->blockItems.push_back(
      new BlockTexOffset(SPRUCE_PLANKS_BLOCK, true, 16.0F, 176.0F));
  this->blockItems.push_back(
      new BlockTexOffset(ACACIA_PLANKS_BLOCK, true, 32.0F, 176.0F));
  this->blockItems.push_back(
      new BlockTexOffset(BIRCH_PLANKS_BLOCK, true, 48.0F, 176.0F));
}

BlockTexOffset* BlockManager::getBlockTexOffsetByType(u8 blockType) {
  for (int i = 0; i < blockItems.size(); i++)
    if (blockItems[i]->blockId == blockType) return blockItems[i];
  return NULL;
}