#include "managers/block_manager.hpp"
#include "file/file_utils.hpp"

using Tyra::FileUtils;
using Tyra::Mesh;
using Tyra::MinecraftPipeline;
using Tyra::Renderer;

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

  // Braking overlay
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_0, true,
                                           mcPip->getTextureOffset() * 0,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_10, true,
                                           mcPip->getTextureOffset() * 1,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_20, true,
                                           mcPip->getTextureOffset() * 2,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_30, true,
                                           mcPip->getTextureOffset() * 3,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_40, true,
                                           mcPip->getTextureOffset() * 4,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_50, true,
                                           mcPip->getTextureOffset() * 5,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_60, true,
                                           mcPip->getTextureOffset() * 6,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_70, true,
                                           mcPip->getTextureOffset() * 7,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_80, true,
                                           mcPip->getTextureOffset() * 8,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_90, true,
                                           mcPip->getTextureOffset() * 9,
                                           mcPip->getTextureOffset() * 15));
  this->blockItems.push_back(new BlockInfo(BLOCK_DAMAGE_100, true,
                                           mcPip->getTextureOffset() * 10,
                                           mcPip->getTextureOffset() * 15));
}

BlockInfo* BlockManager::getBlockTexOffsetByType(const u8& blockType) {
  for (u8 i = 0; i < blockItems.size(); i++)
    if (blockItems[i]->blockId == blockType) return blockItems[i];
  return nullptr;
}