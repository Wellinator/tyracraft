#include "managers/block/texture/block_texture_info_repository.hpp"

BlockTextureRepository::BlockTextureRepository() { loadTextures(); }

BlockTextureRepository::~BlockTextureRepository() { models.clear(); }

BlockInfo* BlockTextureRepository::getTextureInfo(const Blocks& blockType) {
  for (u8 i = 0; i < models.size(); i++)
    if (models[i].blockId == (u8)blockType) return &models[i];

  return nullptr;
}

const u8 BlockTextureRepository::isBlockTransparent(const Blocks& blockType) {
  const BlockInfo* textureInfo = getTextureInfo(blockType);
  if (textureInfo) return textureInfo->_isTransparent;

  TYRA_WARN("isBlockTransparent: Block texture info not found. BlockType -> ",
            std::to_string((u8)blockType).c_str(), " Was it registered?");

  return false;
}

void BlockTextureRepository::loadTextures() {
  // Base Blocks
  models.push_back(BlockInfo(Blocks::STONE_BLOCK, true, {3, 7}, false));
  models.push_back(BlockInfo(Blocks::GRASS_BLOCK, false,
                             {0, 0, 0, 5, 0, 1, 0, 1, 0, 1, 0, 1}, false));
  models.push_back(BlockInfo(Blocks::DIRTY_BLOCK, true, {1, 7}, false));
  models.push_back(BlockInfo(Blocks::WATER_BLOCK, true, {4, 7}, false));
  models.push_back(
      BlockInfo(Blocks::BEDROCK_BLOCK, true, {0, 7}, false, false));
  models.push_back(BlockInfo(Blocks::SAND_BLOCK, true, {2, 7}, false));
  models.push_back(BlockInfo(Blocks::GLASS_BLOCK, true, {0, 9}, true));
  models.push_back(BlockInfo(Blocks::BRICKS_BLOCK, true, {4, 6}, false));

  // Ores and Minerals
  models.push_back(BlockInfo(Blocks::GOLD_ORE_BLOCK, true, {0, 8}, false));
  models.push_back(BlockInfo(Blocks::IRON_ORE_BLOCK, true, {2, 8}, false));
  models.push_back(BlockInfo(Blocks::COAL_ORE_BLOCK, true, {1, 8}, false));
  models.push_back(BlockInfo(Blocks::DIAMOND_ORE_BLOCK, true, {4, 8}, false));
  models.push_back(BlockInfo(Blocks::REDSTONE_ORE_BLOCK, true, {1, 8}, false));
  models.push_back(BlockInfo(Blocks::EMERALD_ORE_BLOCK, true, {3, 8}, false));

  // Wood Planks
  models.push_back(BlockInfo(Blocks::OAK_PLANKS_BLOCK, true, {0, 11}, false));
  models.push_back(
      BlockInfo(Blocks::SPRUCE_PLANKS_BLOCK, true, {1, 11}, false));
  models.push_back(BlockInfo(Blocks::BIRCH_PLANKS_BLOCK, true, {3, 11}, false));
  models.push_back(
      BlockInfo(Blocks::ACACIA_PLANKS_BLOCK, true, {2, 11}, false));

  // Stone bricks
  models.push_back(BlockInfo(Blocks::STONE_BRICK_BLOCK, true, {1, 6}, false));
  models.push_back(
      BlockInfo(Blocks::CRACKED_STONE_BRICKS_BLOCK, true, {0, 6}, false));
  models.push_back(
      BlockInfo(Blocks::MOSSY_STONE_BRICKS_BLOCK, true, {2, 6}, false));
  models.push_back(
      BlockInfo(Blocks::CHISELED_STONE_BRICKS_BLOCK, true, {3, 6}, false));

  // Woods
  models.push_back(BlockInfo(Blocks::OAK_LOG_BLOCK, false,
                             {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}, false));
  models.push_back(BlockInfo(Blocks::OAK_LEAVES_BLOCK, true, {3, 10}, true));

  models.push_back(BlockInfo(Blocks::BIRCH_LOG_BLOCK, false,
                             {6, 0, 6, 0, 6, 1, 6, 1, 6, 1, 6, 1}, false));
  models.push_back(BlockInfo(Blocks::BIRCH_LEAVES_BLOCK, true, {2, 10}, true));
}