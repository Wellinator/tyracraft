#include "managers/block/texture/block_texture_info_repository.hpp"

BlockTextureRepository::BlockTextureRepository(MinecraftPipeline* t_mcPip) {
  this->t_mcPip = t_mcPip;
  this->loadTextures();
}

BlockTextureRepository::~BlockTextureRepository() {
  for (u8 i = 0; i < this->models.size(); i++) {
    delete this->models[i];
    this->models[i] = NULL;
  }
  this->models.clear();
}

BlockInfo* BlockTextureRepository::getTextureInfo(const Blocks& blockType) {
  if ((u8)blockType > (u8)Blocks::AIR_BLOCK &&
      (u8)blockType < (u8)Blocks::TOTAL_OF_BLOCKS) {
    return this->models[(u8)blockType];
  }
  TYRA_WARN("Block texture info not found. BLockType -> ",
            std::to_string((u8)blockType).c_str(), " Was it registered?");
  return nullptr;
}

void BlockTextureRepository::loadTextures() {
  // Non usable indexes
  // Blocks::VOID, Blocks::AIR_BLOCK;
  this->models.push_back(nullptr);
  this->models.push_back(nullptr);

  // Base Blocks
  this->models.push_back(new BlockInfo(Blocks::STONE_BLOCK, true, {3, 7}));
  this->models.push_back(new BlockInfo(Blocks::GRASS_BLOCK, false,
                                       {0, 0, 0, 5, 0, 1, 0, 1, 0, 1, 0, 1}));
  this->models.push_back(new BlockInfo(Blocks::DIRTY_BLOCK, true, {1, 7}));
  this->models.push_back(new BlockInfo(Blocks::WATER_BLOCK, true, {4, 7}));
  this->models.push_back(
      new BlockInfo(Blocks::BEDROCK_BLOCK, true, {0, 7}, false));
  this->models.push_back(new BlockInfo(Blocks::SAND_BLOCK, true, {2, 7}));
  this->models.push_back(new BlockInfo(Blocks::GLASS_BLOCK, true, {0, 9}));
  this->models.push_back(new BlockInfo(Blocks::BRICKS_BLOCK, true, {4, 6}));

  // Ores and Minerals
  this->models.push_back(new BlockInfo(Blocks::GOLD_ORE_BLOCK, true, {0, 8}));
  this->models.push_back(new BlockInfo(Blocks::IRON_ORE_BLOCK, true, {2, 8}));
  this->models.push_back(new BlockInfo(Blocks::COAL_ORE_BLOCK, true, {1, 8}));
  this->models.push_back(
      new BlockInfo(Blocks::DIAMOND_ORE_BLOCK, true, {4, 8}));
  this->models.push_back(
      new BlockInfo(Blocks::REDSTONE_ORE_BLOCK, true, {1, 8}));
  this->models.push_back(
      new BlockInfo(Blocks::EMERALD_ORE_BLOCK, true, {3, 8}));

  // Wood Planks
  this->models.push_back(
      new BlockInfo(Blocks::OAK_PLANKS_BLOCK, true, {0, 11}));
  this->models.push_back(
      new BlockInfo(Blocks::SPRUCE_PLANKS_BLOCK, true, {1, 11}));
  this->models.push_back(
      new BlockInfo(Blocks::BIRCH_PLANKS_BLOCK, true, {3, 11}));
  this->models.push_back(
      new BlockInfo(Blocks::ACACIA_PLANKS_BLOCK, true, {2, 11}));

  // Stone bricks
  this->models.push_back(
      new BlockInfo(Blocks::STONE_BRICK_BLOCK, true, {1, 6}));
  this->models.push_back(
      new BlockInfo(Blocks::CRACKED_STONE_BRICKS_BLOCK, true, {0, 6}));
  this->models.push_back(
      new BlockInfo(Blocks::MOSSY_STONE_BRICKS_BLOCK, true, {2, 6}));
  this->models.push_back(
      new BlockInfo(Blocks::CHISELED_STONE_BRICKS_BLOCK, true, {3, 6}));
  // Stripped Woods
  this->models.push_back(
      new BlockInfo(Blocks::STRIPPED_OAK_WOOD_BLOCK, true, {0, 12}));

  // Woods
  this->models.push_back(new BlockInfo(Blocks::OAK_LOG_BLOCK, false,
                                       {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}));
  this->models.push_back(
      new BlockInfo(Blocks::OAK_LEAVES_BLOCK, true, {3, 10}));
  this->models.push_back(new BlockInfo(Blocks::BIRCH_LOG_BLOCK, false,
                                       {6, 0, 6, 0, 6, 1, 6, 1, 6, 1, 6, 1}));
  this->models.push_back(
      new BlockInfo(Blocks::BIRCH_LEAVES_BLOCK, true, {2, 10}));
}