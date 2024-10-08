#include "managers/block/sound/block_broken_sfx_repository.hpp"

BlockBrokenSfxRepository::BlockBrokenSfxRepository()
    : BlockSfxBaseRepository(SoundFxCategory::Broken) {
  loadModels();
}

BlockBrokenSfxRepository::~BlockBrokenSfxRepository() {
  for (size_t i = 0; i < models.size(); i++) {
    delete models[i];
  }
  models.clear();
  models.shrink_to_fit();
}

void BlockBrokenSfxRepository::loadModels() {
  // Base Blocks
  models.push_back(new SfxBlockModel(Blocks::GRASS_BLOCK, SoundFxCategory::Dig,
                                     SoundFX::Grass1));
  models.push_back(new SfxBlockModel(Blocks::BEDROCK_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::DIRTY_BLOCK, SoundFxCategory::Dig,
                                     SoundFX::Gravel1));
  models.push_back(new SfxBlockModel(Blocks::GRAVEL_BLOCK, SoundFxCategory::Dig,
                                     SoundFX::Gravel1));
  models.push_back(new SfxBlockModel(Blocks::SAND_BLOCK, SoundFxCategory::Dig,
                                     SoundFX::Sand1));
  models.push_back(new SfxBlockModel(Blocks::STONE_BLOCK, SoundFxCategory::Dig,
                                     SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::GLASS_BLOCK,
                                     SoundFxCategory::Random, SoundFX::Glass1));
  models.push_back(new SfxBlockModel(Blocks::PUMPKIN_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));

  // Ores and Minerals
  models.push_back(new SfxBlockModel(Blocks::GOLD_ORE_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::REDSTONE_ORE_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::IRON_ORE_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::EMERALD_ORE_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::DIAMOND_ORE_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::COAL_ORE_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));

  //  Flowers
  models.push_back(new SfxBlockModel(Blocks::POPPY_FLOWER, SoundFxCategory::Dig,
                                     SoundFX::Grass1));
  models.push_back(new SfxBlockModel(Blocks::DANDELION_FLOWER,
                                     SoundFxCategory::Dig, SoundFX::Grass1));
  models.push_back(
      new SfxBlockModel(Blocks::GRASS, SoundFxCategory::Dig, SoundFX::Grass1));

  // Stone bricks
  models.push_back(new SfxBlockModel(Blocks::STONE_BRICK_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::CRACKED_STONE_BRICKS_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::MOSSY_STONE_BRICKS_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::CHISELED_STONE_BRICKS_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::BRICKS_BLOCK, SoundFxCategory::Dig,
                                     SoundFX::Stone1));

  //  Wools
  models.push_back(new SfxBlockModel(Blocks::YELLOW_WOOL,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::BLUE_WOOL,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::GREEN_WOOL,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::ORANGE_WOOL,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::PURPLE_WOOL,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::RED_WOOL, SoundFxCategory::Dig,
                                     SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::BLACK_WOOL,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::WHITE_WOOL,
                                     SoundFxCategory::Dig, SoundFX::Stone1));

  // Woods
  models.push_back(new SfxBlockModel(Blocks::OAK_LOG_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Wood1));
  models.push_back(new SfxBlockModel(Blocks::OAK_LEAVES_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Gravel1));
  models.push_back(new SfxBlockModel(Blocks::BIRCH_LOG_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Wood1));
  models.push_back(new SfxBlockModel(Blocks::BIRCH_LEAVES_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Gravel1));

  // Wood Planks
  models.push_back(new SfxBlockModel(Blocks::OAK_PLANKS_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Wood1));
  models.push_back(new SfxBlockModel(Blocks::SPRUCE_PLANKS_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Wood1));
  models.push_back(new SfxBlockModel(Blocks::ACACIA_PLANKS_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Wood1));
  models.push_back(new SfxBlockModel(Blocks::BIRCH_PLANKS_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Wood1));

  models.push_back(new SfxBlockModel(Blocks::JACK_O_LANTERN_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
  models.push_back(new SfxBlockModel(Blocks::GLOWSTONE_BLOCK,
                                     SoundFxCategory::Dig, SoundFX::Stone1));
}