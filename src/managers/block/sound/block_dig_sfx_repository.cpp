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
  this->models.push_back(new SfxBlockModel(
      Blocks::GRASS_BLOCK, SoundFxCategory::Dig, SoundFX::Grass1));
  this->models.push_back(new SfxBlockModel(
      Blocks::BEDROCK_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::DIRTY_BLOCK, SoundFxCategory::Dig, SoundFX::Gravel1));
  this->models.push_back(new SfxBlockModel(
      Blocks::SAND_BLOCK, SoundFxCategory::Dig, SoundFX::Sand1));
  this->models.push_back(new SfxBlockModel(
      Blocks::STONE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::GLASS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Ores and Minerals
  this->models.push_back(new SfxBlockModel(
      Blocks::GOLD_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::REDSTONE_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::IRON_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::EMERALD_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::DIAMOND_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::COAL_ORE_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Stone bricks
  this->models.push_back(new SfxBlockModel(
      Blocks::STONE_BRICK_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(Blocks::CRACKED_STONE_BRICKS_BLOCK,
                                           SoundFxCategory::Dig,
                                           SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::MOSSY_STONE_BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(Blocks::CHISELED_STONE_BRICKS_BLOCK,
                                           SoundFxCategory::Dig,
                                           SoundFX::Stone1));
  this->models.push_back(new SfxBlockModel(
      Blocks::BRICKS_BLOCK, SoundFxCategory::Dig, SoundFX::Stone1));

  // Woods
  this->models.push_back(new SfxBlockModel(
      Blocks::OAK_LOG_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      Blocks::OAK_LEAVES_BLOCK, SoundFxCategory::Dig, SoundFX::Gravel1));
  this->models.push_back(new SfxBlockModel(
      Blocks::BIRCH_LOG_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      Blocks::BIRCH_LEAVES_BLOCK, SoundFxCategory::Dig, SoundFX::Gravel1));

  // Stripped Woods
  this->models.push_back(new SfxBlockModel(
      Blocks::STRIPPED_OAK_WOOD_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));

  // Wood Planks
  this->models.push_back(new SfxBlockModel(
      Blocks::OAK_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      Blocks::SPRUCE_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      Blocks::ACACIA_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
  this->models.push_back(new SfxBlockModel(
      Blocks::BIRCH_PLANKS_BLOCK, SoundFxCategory::Dig, SoundFX::Wood1));
}