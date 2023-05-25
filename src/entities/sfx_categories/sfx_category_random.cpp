#include "entities/sfx_categories/sfx_category_random.hpp"

SfxRandomCategory::SfxRandomCategory(Audio* t_audio)
    : SfxLibraryCategory(SoundFxCategory::Random) {
  this->t_audio = t_audio;
  this->loadSounds();
};

SfxRandomCategory::~SfxRandomCategory() {
  for (size_t i = 0; i < sounds.size(); i++) {
    delete sounds[i];
  }
  sounds.clear();
  sounds.shrink_to_fit();
};

void SfxRandomCategory::loadSounds() {
  SfxLibrarySound* clickSfx = new SfxLibrarySound(SoundFX::Click);
  clickSfx->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/random/click.adpcm"));
  sounds.push_back(clickSfx);

  SfxLibrarySound* woodClickSfx = new SfxLibrarySound(SoundFX::WoodClick);
  woodClickSfx->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/random/wood_click.adpcm"));
  sounds.push_back(woodClickSfx);

  SfxLibrarySound* glass1Sfx = new SfxLibrarySound(SoundFX::Glass1);
  glass1Sfx->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/random/glass1.adpcm"));
  sounds.push_back(glass1Sfx);
}
