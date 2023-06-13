#include "managers/block/block_info_repository.hpp"

BlockInfoRepository::BlockInfoRepository() { loadBlocksInfo(); }

BlockInfoRepository::~BlockInfoRepository() {
  models.clear();
  models.shrink_to_fit();
}

BlockInfo* BlockInfoRepository::getTextureInfo(const Blocks& blockType) {
  for (u8 i = 0; i < models.size(); i++)
    if (models[i].blockId == (u8)blockType) return &models[i];

  return nullptr;
}

const u8 BlockInfoRepository::isBlockTransparent(const Blocks& blockType) {
  const BlockInfo* textureInfo = getTextureInfo(blockType);
  if (textureInfo) return textureInfo->_isTransparent;

  TYRA_WARN("isBlockTransparent: Block texture info not found. BlockType -> ",
            std::to_string((u8)blockType).c_str(), " Was it registered?");

  return false;
}

void BlockInfoRepository::loadBlocksInfo() {
  // Base Blocks
  models.push_back(BlockInfo(Blocks::STONE_BLOCK, true, {3, 7}, false));
  models.push_back(BlockInfo(Blocks::GRASS_BLOCK, false,
                             {0, 0, 1, 7, 0, 1, 0, 1, 0, 1, 0, 1}, false));
  models.push_back(BlockInfo(Blocks::DIRTY_BLOCK, true, {1, 7}, false));
  models.push_back(
      BlockInfo(Blocks::WATER_BLOCK, true, {4, 7}, true, false, false));
  models.push_back(
      BlockInfo(Blocks::BEDROCK_BLOCK, true, {0, 7}, false, false));
  models.push_back(BlockInfo(Blocks::SAND_BLOCK, true, {2, 7}, false));
  models.push_back(BlockInfo(Blocks::GLASS_BLOCK, true, {0, 9}, true));
  models.push_back(BlockInfo(Blocks::BRICKS_BLOCK, true, {4, 6}, false));
  models.push_back(BlockInfo(Blocks::GRAVEL_BLOCK, true, {5, 6}, false));

  models.push_back(BlockInfo(Blocks::PUMPKIN_BLOCK, false,
                             {1, 13, 1, 13, 0, 13, 0, 13, 3, 13, 0, 13},
                             false));

  // Ores and Minerals
  models.push_back(BlockInfo(Blocks::GOLD_ORE_BLOCK, true, {0, 8}, false));
  models.push_back(BlockInfo(Blocks::REDSTONE_ORE_BLOCK, true, {1, 8}, false));
  models.push_back(BlockInfo(Blocks::IRON_ORE_BLOCK, true, {2, 8}, false));
  models.push_back(BlockInfo(Blocks::EMERALD_ORE_BLOCK, true, {3, 8}, false));
  models.push_back(BlockInfo(Blocks::DIAMOND_ORE_BLOCK, true, {4, 8}, false));
  models.push_back(BlockInfo(Blocks::COAL_ORE_BLOCK, true, {5, 8}, false));

  //  Flowers
  models.push_back(
      BlockInfo(Blocks::GRASS, true, {9, 10}, true, true, false, true));
  models.push_back(
      BlockInfo(Blocks::POPPY_FLOWER, true, {7, 10}, true, true, false, true));
  models.push_back(BlockInfo(Blocks::DANDELION_FLOWER, true, {8, 10}, true,
                             true, false, true));

  // Wood Planks
  models.push_back(BlockInfo(Blocks::OAK_PLANKS_BLOCK, true, {0, 11}, false));
  models.push_back(
      BlockInfo(Blocks::SPRUCE_PLANKS_BLOCK, true, {1, 11}, false));
  models.push_back(BlockInfo(Blocks::BIRCH_PLANKS_BLOCK, true, {3, 11}, false));
  models.push_back(
      BlockInfo(Blocks::ACACIA_PLANKS_BLOCK, true, {2, 11}, false));

  // Light Emissor
  models.push_back(
      BlockInfo(Blocks::GLOWSTONE_BLOCK, true, {4, 13}, false, true, true));
  models.push_back(BlockInfo(Blocks::JACK_O_LANTERN_BLOCK, false,
                             {1, 13, 1, 13, 0, 13, 0, 13, 2, 13, 0, 13},
                             false));

  // Stone bricks
  models.push_back(BlockInfo(Blocks::STONE_BRICK_BLOCK, true, {1, 6}, false));
  models.push_back(
      BlockInfo(Blocks::CRACKED_STONE_BRICKS_BLOCK, true, {0, 6}, false));
  models.push_back(
      BlockInfo(Blocks::MOSSY_STONE_BRICKS_BLOCK, true, {2, 6}, false));
  models.push_back(
      BlockInfo(Blocks::CHISELED_STONE_BRICKS_BLOCK, true, {3, 6}, false));

  // Concretes
  models.push_back(BlockInfo(Blocks::YELLOW_CONCRETE, true, {6, 6}, false));
  models.push_back(BlockInfo(Blocks::BLUE_CONCRETE, true, {7, 6}, false));
  models.push_back(BlockInfo(Blocks::GREEN_CONCRETE, true, {8, 6}, false));
  models.push_back(BlockInfo(Blocks::ORANGE_CONCRETE, true, {9, 6}, false));
  models.push_back(BlockInfo(Blocks::PURPLE_CONCRETE, true, {10, 6}, false));
  models.push_back(BlockInfo(Blocks::RED_CONCRETE, true, {11, 6}, false));
  models.push_back(BlockInfo(Blocks::WHITE_CONCRETE, true, {12, 6}, false));
  models.push_back(BlockInfo(Blocks::BLACK_CONCRETE, true, {13, 6}, false));

  // Woods
  models.push_back(BlockInfo(Blocks::OAK_LOG_BLOCK, false,
                             {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}, false));
  models.push_back(BlockInfo(Blocks::OAK_LEAVES_BLOCK, true, {3, 10}, true));

  models.push_back(BlockInfo(Blocks::BIRCH_LOG_BLOCK, false,
                             {6, 0, 6, 0, 6, 1, 6, 1, 6, 1, 6, 1}, false));
  models.push_back(BlockInfo(Blocks::BIRCH_LEAVES_BLOCK, true, {2, 10}, true));
}