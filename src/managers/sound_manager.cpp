#include "managers/sound_manager.hpp"

SoundManager::SoundManager(Engine* t_engine) {
  this->t_engine = t_engine;
  this->soundLibrary = new SfxLibrary(&t_engine->audio);
}

SoundManager::~SoundManager() { delete soundLibrary; }

void SoundManager::playSfx(const SoundFxCategory& idCategory,
                           const SoundFX& idSound) {
  return;
  this->playSfx(idCategory, idSound, getAvailableChannel());
};

void SoundManager::playSfx(const SoundFxCategory& idCategory,
                           const SoundFX& idSound, const s8& t_ch) {
  return;
  SfxLibrarySound* t_sound = this->getSound(idCategory, idSound);
  if (t_sound) this->t_engine->audio.adpcm.tryPlay(t_sound->_sound, t_ch);
};

void SoundManager::setSfxVolume(const u8& t_vol, const s8& t_ch) {
  return this->t_engine->audio.adpcm.setVolume(t_vol, t_ch);
};

const int SoundManager::getAvailableChannel() {
  currentAdpcmChannel %= MAX_ADPCM_CH;
  return currentAdpcmChannel++;
};
