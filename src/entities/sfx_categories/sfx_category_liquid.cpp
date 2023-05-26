#include "entities/sfx_categories/sfx_category_liquid.hpp"

SfxLiquidCategory::SfxLiquidCategory(Audio* t_audio)
    : SfxLibraryCategory(SoundFxCategory::Liquid) {
  this->t_audio = t_audio;
  this->loadSounds();
};

SfxLiquidCategory::~SfxLiquidCategory() {
  for (size_t i = 0; i < sounds.size(); i++) {
    delete sounds[i];
  }
  sounds.clear();
  sounds.shrink_to_fit();
};

void SfxLiquidCategory::loadSounds() {
  SfxLibrarySound* splash = new SfxLibrarySound(SoundFX::Splash);
  splash->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/liquid/splash.adpcm"));
  sounds.push_back(splash);

  SfxLibrarySound* splash1 = new SfxLibrarySound(SoundFX::Splash1);
  splash1->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/liquid/splash1.adpcm"));
  sounds.push_back(splash1);

  SfxLibrarySound* splash2 = new SfxLibrarySound(SoundFX::Splash2);
  splash2->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/liquid/splash2.adpcm"));
  sounds.push_back(splash2);

  SfxLibrarySound* swim1 = new SfxLibrarySound(SoundFX::Swim1);
  swim1->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/liquid/swim1.adpcm"));
  sounds.push_back(swim1);

  SfxLibrarySound* swim2 = new SfxLibrarySound(SoundFX::Swim2);
  swim2->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/liquid/swim2.adpcm"));
  sounds.push_back(swim2);

  SfxLibrarySound* swim3 = new SfxLibrarySound(SoundFX::Swim3);
  swim3->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/liquid/swim3.adpcm"));
  sounds.push_back(swim3);

  SfxLibrarySound* swim4 = new SfxLibrarySound(SoundFX::Swim4);
  swim4->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/liquid/swim4.adpcm"));
  sounds.push_back(swim4);

  SfxLibrarySound* water = new SfxLibrarySound(SoundFX::Water);
  water->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/liquid/water.adpcm"));
  sounds.push_back(water);
}
