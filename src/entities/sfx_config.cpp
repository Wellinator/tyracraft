#include "entities/sfx_config.hpp"

SfxConfig::SfxConfig() {}

SfxConfig::~SfxConfig() {}

SfxConfigModel* SfxConfig::getPlaceSoundConfig(Blocks blockType) {
  SfxConfigModel* model = new SfxConfigModel();

  switch (blockType) {
    case Blocks::GRASS_BLOCK:
      model->_pitch = 80;
      model->_volume = 100;
      break;

    case Blocks::DIRTY_BLOCK:
      model->_pitch = 80;
      model->_volume = 100;
      break;

    case Blocks::STONE_BLOCK:
      model->_pitch = 80;
      model->_volume = 100;
      break;

    default:
      model->_pitch = 80;
      model->_volume = 100;
      break;
  }

  return model;
}

SfxConfigModel* SfxConfig::getStepSoundConfig(Blocks blockType) {
  SfxConfigModel* model = new SfxConfigModel();

  switch (blockType) {
    case Blocks::GRASS_BLOCK:
      model->_pitch = 15;
      model->_volume = 100;
      break;

    case Blocks::DIRTY_BLOCK:
      model->_pitch = 15;
      model->_volume = 100;
      break;

    case Blocks::STONE_BLOCK:
      model->_pitch = 15;
      model->_volume = 100;
      break;

    default:
      model->_pitch = 15;
      model->_volume = 100;
      break;
  }

  return model;
}

SfxConfigModel* SfxConfig::getBreakingSoundConfig(Blocks blockType) {
  SfxConfigModel* model = new SfxConfigModel();

  switch (blockType) {
    case Blocks::GRASS_BLOCK:
      model->_pitch = 25;
      model->_volume = 50;
      break;

    case Blocks::DIRTY_BLOCK:
      model->_pitch = 25;
      model->_volume = 50;
      break;

    case Blocks::STONE_BLOCK:
      model->_pitch = 25;
      model->_volume = 50;
      break;

    default:
      model->_pitch = 25;
      model->_volume = 50;
      break;
  }

  return model;
}

SfxConfigModel* SfxConfig::getBrokenSoundConfig(Blocks blockType) {
  SfxConfigModel* model = new SfxConfigModel();

  switch (blockType) {
    case Blocks::GRASS_BLOCK:
      model->_pitch = 80;
      model->_volume = 100;
      break;

    case Blocks::DIRTY_BLOCK:
      model->_pitch = 80;
      model->_volume = 100;
      break;

    case Blocks::STONE_BLOCK:
      model->_pitch = 80;
      model->_volume = 100;
      break;

    default:
      model->_pitch = 80;
      model->_volume = 100;
      break;
  }

  return model;
}