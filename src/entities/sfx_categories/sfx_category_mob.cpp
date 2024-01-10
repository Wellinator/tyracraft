#include "entities/sfx_categories/sfx_category_mob.hpp"

SfxMobCategory::SfxMobCategory(Audio* t_audio)
    : SfxLibraryCategory(SoundFxCategory::Mob) {
  this->t_audio = t_audio;
  this->loadSounds();
};

SfxMobCategory::~SfxMobCategory() {
  for (size_t i = 0; i < sounds.size(); i++) {
    delete sounds[i];
  }
  sounds.clear();
  sounds.shrink_to_fit();
};

void SfxMobCategory::loadSounds() { loadPigSounds(); }

void SfxMobCategory::loadPigSounds() {
  SfxLibrarySound* death = new SfxLibrarySound(SoundFX::PigDeath);
  death->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/death.adpcm"));
  sounds.push_back(death);

  SfxLibrarySound* say1 = new SfxLibrarySound(SoundFX::PigSay1);
  say1->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/say1.adpcm"));
  sounds.push_back(say1);

  SfxLibrarySound* say2 = new SfxLibrarySound(SoundFX::PigSay2);
  say2->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/say2.adpcm"));
  sounds.push_back(say2);

  SfxLibrarySound* say3 = new SfxLibrarySound(SoundFX::PigSay3);
  say3->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/say3.adpcm"));
  sounds.push_back(say3);

  SfxLibrarySound* step1 = new SfxLibrarySound(SoundFX::PigStep1);
  step1->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/step1.adpcm"));
  sounds.push_back(step1);

  SfxLibrarySound* step2 = new SfxLibrarySound(SoundFX::PigStep2);
  step2->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/step2.adpcm"));
  sounds.push_back(step2);

  SfxLibrarySound* step3 = new SfxLibrarySound(SoundFX::PigStep3);
  step3->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/step3.adpcm"));
  sounds.push_back(step3);

  SfxLibrarySound* step4 = new SfxLibrarySound(SoundFX::PigStep4);
  step4->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/step4.adpcm"));
  sounds.push_back(step4);

  SfxLibrarySound* step5 = new SfxLibrarySound(SoundFX::PigStep5);
  step5->_sound = this->t_audio->adpcm.load(
      FileUtils::fromCwd("sounds/mob/pig/step5.adpcm"));
  sounds.push_back(step5);
}