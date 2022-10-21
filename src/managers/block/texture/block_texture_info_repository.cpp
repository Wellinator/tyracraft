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

BlockInfo* BlockTextureRepository::getTextureInfo(const u8& blockType) {
  for (u8 i = 0; i < this->models.size(); i++)
    if (this->models[i]->blockId == blockType) return this->models[i];
  TYRA_WARN("Block texture info not found. BLockType -> ",
            std::to_string(blockType).c_str(), " Was it registered?");
  return nullptr;
}

void BlockTextureRepository::loadTextures() {
  // Base Blocks
  this->models.push_back(new BlockInfo(GRASS_BLOCK, false,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 0));
  this->models.push_back(new BlockInfo(BEDROCK_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(DIRTY_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(SAND_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(STONE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(WATER_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 4,
                                       this->t_mcPip->getTextureOffset() * 7));
  this->models.push_back(new BlockInfo(GLASS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 9));

  // Ores and Minerals
  this->models.push_back(new BlockInfo(GOLD_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(REDSTONE_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(IRON_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(EMERALD_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(DIAMOND_ORE_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 4,
                                       this->t_mcPip->getTextureOffset() * 8));
  this->models.push_back(new BlockInfo(COAL_ORE_BLOCK, true, 80.0F,
                                       this->t_mcPip->getTextureOffset() * 8));

  // Stone bricks
  this->models.push_back(new BlockInfo(STONE_BRICK_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 6));
  this->models.push_back(new BlockInfo(CRACKED_STONE_BRICKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 6));
  this->models.push_back(new BlockInfo(MOSSY_STONE_BRICKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 6));
  this->models.push_back(new BlockInfo(CHISELED_STONE_BRICKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 6));
  this->models.push_back(new BlockInfo(BRICKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 4,
                                       this->t_mcPip->getTextureOffset() * 6));

  // Woods
  this->models.push_back(new BlockInfo(OAK_LOG_BLOCK, false,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 0));
  this->models.push_back(new BlockInfo(OAK_LEAVES_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 10));

  // Stripped Woods
  this->models.push_back(new BlockInfo(STRIPPED_OAK_WOOD_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 12));

  // Wood Planks
  this->models.push_back(new BlockInfo(OAK_PLANKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 0,
                                       this->t_mcPip->getTextureOffset() * 11));
  this->models.push_back(new BlockInfo(SPRUCE_PLANKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 1,
                                       this->t_mcPip->getTextureOffset() * 11));
  this->models.push_back(new BlockInfo(ACACIA_PLANKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 2,
                                       this->t_mcPip->getTextureOffset() * 11));
  this->models.push_back(new BlockInfo(BIRCH_PLANKS_BLOCK, true,
                                       this->t_mcPip->getTextureOffset() * 3,
                                       this->t_mcPip->getTextureOffset() * 11));
}