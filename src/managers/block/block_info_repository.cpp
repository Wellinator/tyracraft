#include "managers/block/block_info_repository.hpp"

BlockInfoRepository::BlockInfoRepository() { loadBlocksInfo(); }

BlockInfoRepository::~BlockInfoRepository() {
  models.clear();
  models.shrink_to_fit();
}

BlockInfo* BlockInfoRepository::getTextureInfo(const Blocks& blockType) {
  return (u8)blockType < (u8)Blocks::TOTAL_OF_BLOCKS ? &models[(u8)blockType]
                                                     : nullptr;
}

const u8 BlockInfoRepository::isBlockTransparent(const Blocks& blockType) {
  const BlockInfo* textureInfo = getTextureInfo(blockType);
  if (textureInfo) return textureInfo->_isTransparent;

  TYRA_WARN("isBlockTransparent: Block texture info not found. BlockType -> ",
            std::to_string((u8)blockType).c_str(), " Was it registered?");

  return false;
}

void BlockInfoRepository::loadBlocksInfo() {
  models.reserve(static_cast<u16>(Blocks::TOTAL_OF_BLOCKS));

  // Base Blocks
  models[(u8)Blocks::STONE_BLOCK] =
      (BlockInfo(Blocks::STONE_BLOCK, true, {115}, 1.5F, false));
  models[(u8)Blocks::GRASS_BLOCK] = (BlockInfo(
      Blocks::GRASS_BLOCK, false, {0, 113, 16, 16, 16, 16}, 0.6F, false));
  models[(u8)Blocks::DIRTY_BLOCK] =
      (BlockInfo(Blocks::DIRTY_BLOCK, true, {113}, 0.5F, false));
  models[(u8)Blocks::WATER_BLOCK] =
      (BlockInfo(Blocks::WATER_BLOCK, true, {116}, 0, true, false, false));
  models[(u8)Blocks::BEDROCK_BLOCK] =
      (BlockInfo(Blocks::BEDROCK_BLOCK, true, {112}, -1.0F, false, false));
  models[(u8)Blocks::SAND_BLOCK] =
      (BlockInfo(Blocks::SAND_BLOCK, true, {114}, 0.5F, false));
  models[(u8)Blocks::GLASS_BLOCK] =
      (BlockInfo(Blocks::GLASS_BLOCK, true, {144}, 0.3F, true));
  models[(u8)Blocks::BRICKS_BLOCK] =
      (BlockInfo(Blocks::BRICKS_BLOCK, true, {100}, 2.0F, false));
  models[(u8)Blocks::GRAVEL_BLOCK] =
      (BlockInfo(Blocks::GRAVEL_BLOCK, true, {101}, 0.6F, false));

  models[(u8)Blocks::PUMPKIN_BLOCK] =
      (BlockInfo(Blocks::PUMPKIN_BLOCK, false, {209, 209, 208, 208, 211, 208},
                 1.0F, false));

  // Ores and Minerals
  models[(u8)Blocks::GOLD_ORE_BLOCK] =
      (BlockInfo(Blocks::GOLD_ORE_BLOCK, true, {128}, 3.0F, false));
  models[(u8)Blocks::REDSTONE_ORE_BLOCK] =
      (BlockInfo(Blocks::REDSTONE_ORE_BLOCK, true, {129}, 3.0F, false));
  models[(u8)Blocks::IRON_ORE_BLOCK] =
      (BlockInfo(Blocks::IRON_ORE_BLOCK, true, {130}, 3.0F, false));
  models[(u8)Blocks::EMERALD_ORE_BLOCK] =
      (BlockInfo(Blocks::EMERALD_ORE_BLOCK, true, {131}, 3.0F, false));
  models[(u8)Blocks::DIAMOND_ORE_BLOCK] =
      (BlockInfo(Blocks::DIAMOND_ORE_BLOCK, true, {132}, 3.0F, false));
  models[(u8)Blocks::COAL_ORE_BLOCK] =
      (BlockInfo(Blocks::COAL_ORE_BLOCK, true, {133}, 3.0F, false));

  //  Flowers
  models[(u8)Blocks::GRASS] =
      (BlockInfo(Blocks::GRASS, true, {169}, 0.0F, true, true, false, true));
  models[(u8)Blocks::POPPY_FLOWER] = (BlockInfo(
      Blocks::POPPY_FLOWER, true, {167}, 0.0F, true, true, false, true));
  models[(u8)Blocks::DANDELION_FLOWER] = (BlockInfo(
      Blocks::DANDELION_FLOWER, true, {168}, 0.0F, true, true, false, true));

  // Wood Planks
  models[(u8)Blocks::OAK_PLANKS_BLOCK] =
      (BlockInfo(Blocks::OAK_PLANKS_BLOCK, true, {176}, 2.0F, false));
  models[(u8)Blocks::SPRUCE_PLANKS_BLOCK] =
      (BlockInfo(Blocks::SPRUCE_PLANKS_BLOCK, true, {177}, 2.0F, false));
  models[(u8)Blocks::ACACIA_PLANKS_BLOCK] =
      (BlockInfo(Blocks::ACACIA_PLANKS_BLOCK, true, {178}, 2.0F, false));
  models[(u8)Blocks::BIRCH_PLANKS_BLOCK] =
      (BlockInfo(Blocks::BIRCH_PLANKS_BLOCK, true, {179}, 2.0F, false));

  // Light Emissor
  models[(u8)Blocks::GLOWSTONE_BLOCK] = (BlockInfo(
      Blocks::GLOWSTONE_BLOCK, true, {212}, 0.3F, false, true, true));
  models[(u8)Blocks::JACK_O_LANTERN_BLOCK] =
      (BlockInfo(Blocks::JACK_O_LANTERN_BLOCK, false,
                 {209, 209, 208, 208, 210, 208}, 1.0F, false));

  // Stone bricks
  models[(u8)Blocks::CRACKED_STONE_BRICKS_BLOCK] =
      (BlockInfo(Blocks::CRACKED_STONE_BRICKS_BLOCK, true, {96}, 1.5F, false));
  models[(u8)Blocks::STONE_BRICK_BLOCK] =
      (BlockInfo(Blocks::STONE_BRICK_BLOCK, true, {97}, 1.5F, false));
  models[(u8)Blocks::MOSSY_STONE_BRICKS_BLOCK] =
      (BlockInfo(Blocks::MOSSY_STONE_BRICKS_BLOCK, true, {98}, 1.5F, false));
  models[(u8)Blocks::CHISELED_STONE_BRICKS_BLOCK] =
      (BlockInfo(Blocks::CHISELED_STONE_BRICKS_BLOCK, true, {99}, 1.5F, false));

  // Concretes
  models[(u8)Blocks::YELLOW_CONCRETE] =
      (BlockInfo(Blocks::YELLOW_CONCRETE, true, {102}, 1.8F, false));
  models[(u8)Blocks::BLUE_CONCRETE] =
      (BlockInfo(Blocks::BLUE_CONCRETE, true, {103}, 1.8F, false));
  models[(u8)Blocks::GREEN_CONCRETE] =
      (BlockInfo(Blocks::GREEN_CONCRETE, true, {104}, 1.8F, false));
  models[(u8)Blocks::ORANGE_CONCRETE] =
      (BlockInfo(Blocks::ORANGE_CONCRETE, true, {105}, 1.8F, false));
  models[(u8)Blocks::PURPLE_CONCRETE] =
      (BlockInfo(Blocks::PURPLE_CONCRETE, true, {106}, 1.8F, false));
  models[(u8)Blocks::RED_CONCRETE] =
      (BlockInfo(Blocks::RED_CONCRETE, true, {107}, 1.8F, false));
  models[(u8)Blocks::WHITE_CONCRETE] =
      (BlockInfo(Blocks::WHITE_CONCRETE, true, {108}, 1.8F, false));
  models[(u8)Blocks::BLACK_CONCRETE] =
      (BlockInfo(Blocks::BLACK_CONCRETE, true, {109}, 1.8F, false));

  // Logs
  models[(u8)Blocks::OAK_LOG_BLOCK] = (BlockInfo(
      Blocks::OAK_LOG_BLOCK, false, {1, 1, 17, 17, 17, 17}, 2.0F, false));
  models[(u8)Blocks::BIRCH_LOG_BLOCK] = (BlockInfo(
      Blocks::BIRCH_LOG_BLOCK, false, {6, 6, 0, 22, 22, 22, 22}, 2.0F, false));
  // Leaves
  models[(u8)Blocks::BIRCH_LEAVES_BLOCK] =
      (BlockInfo(Blocks::BIRCH_LEAVES_BLOCK, true, {162}, 0.2F, true));
  models[(u8)Blocks::OAK_LEAVES_BLOCK] =
      (BlockInfo(Blocks::OAK_LEAVES_BLOCK, true, {163}, 0.2F, true));
}