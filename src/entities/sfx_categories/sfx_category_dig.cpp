#include "entities/sfx_categories/sfx_category_dig.hpp"

SfxDigCategory::SfxDigCategory(Audio* t_audio)
    : SfxLibraryCategory(SoundFxCategory::Dig) {
  this->t_audio = t_audio;
  this->loadSounds();
};

SfxDigCategory::~SfxDigCategory() {
  for (size_t i = 0; i < sounds.size(); i++) {
    delete sounds[i];
  }
  sounds.clear();
};

void SfxDigCategory::loadSounds() {
  SfxLibrarySound* grass1 = new SfxLibrarySound(SoundFX::Grass1);
  grass1->_sound =
      this->t_audio->adpcm.load(FileUtils::fromCwd("sounds/dig/grass1.adpcm"));
  sounds.push_back(grass1);

  SfxLibrarySound* gravel1 = new SfxLibrarySound(SoundFX::Gravel1);
  gravel1->_sound =
      this->t_audio->adpcm.load(FileUtils::fromCwd("sounds/dig/gravel1.adpcm"));
  sounds.push_back(gravel1);

  SfxLibrarySound* sand1 = new SfxLibrarySound(SoundFX::Sand1);
  sand1->_sound =
      this->t_audio->adpcm.load(FileUtils::fromCwd("sounds/dig/sand1.adpcm"));
  sounds.push_back(sand1);

  SfxLibrarySound* snow1 = new SfxLibrarySound(SoundFX::Snow1);
  snow1->_sound =
      this->t_audio->adpcm.load(FileUtils::fromCwd("sounds/dig/snow1.adpcm"));
  sounds.push_back(snow1);

  SfxLibrarySound* stone1 = new SfxLibrarySound(SoundFX::Stone1);
  stone1->_sound =
      this->t_audio->adpcm.load(FileUtils::fromCwd("sounds/dig/stone1.adpcm"));
  sounds.push_back(stone1);

  SfxLibrarySound* wetGrass1 = new SfxLibrarySound(SoundFX::WetGrass1);
  wetGrass1->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/dig/wet_grass1.adpcm"));
  sounds.push_back(wetGrass1);

  SfxLibrarySound* wood1 = new SfxLibrarySound(SoundFX::Wood1);
  wood1->_sound =
      this->t_audio->adpcm.load(FileUtils::fromCwd("sounds/dig/wood1.adpcm"));
  sounds.push_back(wood1);
}
