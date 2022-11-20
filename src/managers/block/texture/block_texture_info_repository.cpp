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
  for (u8 i = 0; i < this->models.size(); i++)
    if (this->models[i]->blockId == (u8)blockType) return this->models[i];
  TYRA_WARN("Block texture info not found. BLockType -> ",
            std::to_string((u8)blockType).c_str(), " Was it registered?");
  return nullptr;
}

void BlockTextureRepository::loadTextures() {
  // Base Blocks
  this->models.push_back(new BlockInfo(Blocks::GRASS_BLOCK, false,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 0));
  this->models.push_back(new BlockInfo(
      Blocks::BEDROCK_BLOCK, true, this->t_mcPip->getTextureOffset() * 0,
      this->t_mcPip->getTextureOffset() * 7, false));

  this->models.push_back(new BlockInfo(Blocks::DIRTY_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(Blocks::SAND_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(Blocks::STONE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(Blocks::WATER_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 4,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(Blocks::GLASS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 9));

  // Ores and Minerals
  this->models.push_back(new BlockInfo(Blocks::GOLD_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(Blocks::REDSTONE_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(Blocks::IRON_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(Blocks::EMERALD_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(Blocks::DIAMOND_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 4,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(Blocks::COAL_ORE_BLOCK, true, 80.0F,
                                       this->t_mcPip->getTextureOffset() * 8));

  // Stone bricks
  this->models.push_back(new BlockInfo(Blocks::STONE_BRICK_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 6));
  this->models.push_back(new BlockInfo(Blocks::CRACKED_STONE_BRICKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 6));
  this->models.push_back(new BlockInfo(Blocks::MOSSY_STONE_BRICKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 6));
  this->models.push_back(new BlockInfo(Blocks::CHISELED_STONE_BRICKS_BLOCK,
                                       true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 6));
  this->models.push_back(new BlockInfo(Blocks::BRICKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 4,
                                       this->t_mcPip->getTextureOffset() * 6));

  // Woods
  this->models.push_back(new BlockInfo(Blocks::OAK_LOG_BLOCK, false,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 0));
  this->models.push_back(new BlockInfo(Blocks::OAK_LEAVES_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 10));
  this->models.push_back(new BlockInfo(Blocks::BIRCH_LOG_BLOCK, false,
                                       this->t_mcPip->getTextureOffset() * 6,
                                       this->t_mcPip->getTextureOffset() * 0));
  this->models.push_back(new BlockInfo(Blocks::BIRCH_LEAVES_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 10));

  // Stripped Woods
  this->models.push_back(new BlockInfo(Blocks::STRIPPED_OAK_WOOD_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 12));

  // Wood Planks
  this->models.push_back(new BlockInfo(Blocks::OAK_PLANKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 11));
  this->models.push_back(new BlockInfo(Blocks::SPRUCE_PLANKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 11));
  this->models.push_back(new BlockInfo(Blocks::ACACIA_PLANKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 11));
  this->models.push_back(new BlockInfo(Blocks::BIRCH_PLANKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 11));
}