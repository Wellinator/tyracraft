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
  models.push_back(BlockInfo(Blocks::STONE_BLOCK, true, {115}, 1.5F, false));
  models.push_back(BlockInfo(Blocks::GRASS_BLOCK, false,
                             {0, 113, 16, 16, 16, 16}, 0.6F, false));
  models.push_back(BlockInfo(Blocks::DIRTY_BLOCK, true, {113}, 0.5F, false));
  models.push_back(
      BlockInfo(Blocks::WATER_BLOCK, true, {116}, 0, true, false, false));
  models.push_back(
      BlockInfo(Blocks::BEDROCK_BLOCK, true, {112}, -1.0F, false, false));
  models.push_back(BlockInfo(Blocks::SAND_BLOCK, true, {114}, 0.5F, false));
  models.push_back(BlockInfo(Blocks::GLASS_BLOCK, true, {144}, 0.3F, true));
  models.push_back(BlockInfo(Blocks::BRICKS_BLOCK, true, {100}, 2.0F, false));
  models.push_back(BlockInfo(Blocks::GRAVEL_BLOCK, true, {101}, 0.6F, false));

  models.push_back(BlockInfo(Blocks::PUMPKIN_BLOCK, false,
                             {209, 209, 208, 208, 211, 208}, 1.0F, false));

  // Ores and Minerals
  models.push_back(BlockInfo(Blocks::GOLD_ORE_BLOCK, true, {128}, 3.0F, false));
  models.push_back(
      BlockInfo(Blocks::REDSTONE_ORE_BLOCK, true, {129}, 3.0F, false));
  models.push_back(BlockInfo(Blocks::IRON_ORE_BLOCK, true, {130}, 3.0F, false));
  models.push_back(
      BlockInfo(Blocks::EMERALD_ORE_BLOCK, true, {131}, 3.0F, false));
  models.push_back(
      BlockInfo(Blocks::DIAMOND_ORE_BLOCK, true, {132}, 3.0F, false));
  models.push_back(BlockInfo(Blocks::COAL_ORE_BLOCK, true, {133}, 3.0F, false));

  //  Flowers
  models.push_back(
      BlockInfo(Blocks::GRASS, true, {169}, 0.0F, true, true, false, true));
  models.push_back(BlockInfo(Blocks::POPPY_FLOWER, true, {167}, 0.0F, true,
                             true, false, true));
  models.push_back(BlockInfo(Blocks::DANDELION_FLOWER, true, {168}, 0.0F, true,
                             true, false, true));

  // Wood Planks
  models.push_back(
      BlockInfo(Blocks::OAK_PLANKS_BLOCK, true, {176}, 2.0F, false));
  models.push_back(
      BlockInfo(Blocks::SPRUCE_PLANKS_BLOCK, true, {177}, 2.0F, false));
  models.push_back(
      BlockInfo(Blocks::ACACIA_PLANKS_BLOCK, true, {178}, 2.0F, false));
  models.push_back(
      BlockInfo(Blocks::BIRCH_PLANKS_BLOCK, true, {179}, 2.0F, false));

  // Light Emissor
  models.push_back(
      BlockInfo(Blocks::GLOWSTONE_BLOCK, true, {212}, 0.3F, false, true, true));
  models.push_back(BlockInfo(Blocks::JACK_O_LANTERN_BLOCK, false,
                             {209, 209, 208, 208, 210, 208}, 1.0F, false));

  // Stone bricks
  models.push_back(
      BlockInfo(Blocks::CRACKED_STONE_BRICKS_BLOCK, true, {96}, 1.5F, false));
  models.push_back(
      BlockInfo(Blocks::STONE_BRICK_BLOCK, true, {97}, 1.5F, false));
  models.push_back(
      BlockInfo(Blocks::MOSSY_STONE_BRICKS_BLOCK, true, {98}, 1.5F, false));
  models.push_back(
      BlockInfo(Blocks::CHISELED_STONE_BRICKS_BLOCK, true, {99}, 1.5F, false));

  // Concretes
  models.push_back(
      BlockInfo(Blocks::YELLOW_CONCRETE, true, {102}, 1.8F, false));
  models.push_back(BlockInfo(Blocks::BLUE_CONCRETE, true, {103}, 1.8F, false));
  models.push_back(BlockInfo(Blocks::GREEN_CONCRETE, true, {104}, 1.8F, false));
  models.push_back(
      BlockInfo(Blocks::ORANGE_CONCRETE, true, {105}, 1.8F, false));
  models.push_back(
      BlockInfo(Blocks::PURPLE_CONCRETE, true, {106}, 1.8F, false));
  models.push_back(BlockInfo(Blocks::RED_CONCRETE, true, {107}, 1.8F, false));
  models.push_back(BlockInfo(Blocks::WHITE_CONCRETE, true, {108}, 1.8F, false));
  models.push_back(BlockInfo(Blocks::BLACK_CONCRETE, true, {109}, 1.8F, false));

  // Logs
  models.push_back(BlockInfo(Blocks::OAK_LOG_BLOCK, false,
                             {1, 1, 17, 17, 17, 17}, 2.0F, false));
  models.push_back(BlockInfo(Blocks::BIRCH_LOG_BLOCK, false,
                             {6, 6, 0, 22, 22, 22, 22}, 2.0F, false));
  // Leaves
  models.push_back(
      BlockInfo(Blocks::BIRCH_LEAVES_BLOCK, true, {162}, 0.2F, true));
  models.push_back(
      BlockInfo(Blocks::OAK_LEAVES_BLOCK, true, {163}, 0.2F, true));
}