#include "managers/block/sound/block_dig_sfx_repository.hpp"

BlockDigSfxRepository::BlockDigSfxRepository()
    : BlockSfxBaseRepository(SoundFxCategory::Dig) {
  this->loadModels();
}

BlockDigSfxRepository::~BlockDigSfxRepository() {
  for (size_t i = 0; i < this->models.size(); i++) {
    delete this->models[i];
  }
  this->models.clear();
}

void BlockDigSfxRepository::loadModels() {
  // Base Blocks
  this->models.push_back(
      new SfxBlockModel(GRASS_BLOCK, SoundFxCategory::Dig, SoundFX::Grass1));
  this->models.push_back(
      new SfxBlockModel(BEDROCK_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(
      new SfxBlockModel(DIRTY_BLOCK, SoundFxCategory::Dig, SoundFX::Gravel1));
  this->models.push_back(
      new SfxBlockModel(SAND_BLOCK, SoundFxCategory::Dig, SoundFX::Sand1));
  this->models.push_back(
      new SfxBlockModel(STONE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(
      new SfxBlockModel(GLASS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Ores and Minerals
  this->models.push_back(
      new SfxBlockModel(GOLD_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      REDSTONE_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(
      new SfxBlockModel(IRON_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      EMERALD_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      DIAMOND_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(
      new SfxBlockModel(COAL_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Stone bricks
  this->models.push_back(new SfxBlockModel(
      STONE_BRICK_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      CRACKED_STONE_BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      MOSSY_STONE_BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      CHISELED_STONE_BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(
      new SfxBlockModel(BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Woods
  this->models.push_back(
      new SfxBlockModel(OAK_LOG_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      OAK_LEAVES_BLOCK, SoundFxCategory::Dig, SoundFX::Gravel1));

  // Stripped Woods
  this->models.push_back(new SfxBlockModel(
      STRIPPED_OAK_WOOD_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));

  // Wood Planks
  this->models.push_back(new SfxBlockModel(
      OAK_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      SPRUCE_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      ACACIA_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      BIRCH_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
}