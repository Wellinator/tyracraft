#include "managers/sound_manager.hpp"

SoundManager::SoundManager(Engine* t_engine) {
  this->t_engine = t_engine;
  this->soundLibrary = new SfxLibrary(&t_engine->audio);
}

SoundManager::~SoundManager() { delete soundLibrary; }

AdpcmResult SoundManager::playSfx(SoundFxCategory idCategory, SoundFX idSound) {
  return this->playSfx(idCategory, idSound, getAvailableChannel());
};

AdpcmResult SoundManager::playSfx(SoundFxCategory idCategory, SoundFX idSound,
                                  const s8& t_ch) {
  SfxLibrarySound* t_sound = this->getSound(idCategory, idSound);
  TYRA_ASSERT(t_sound, "Sound FX not found in sound library");
  const AdpcmResult result =
      this->t_engine->audio.adpcm.tryPlay(t_sound->_sound, t_ch);
  Tyra::Threading::switchThread();
  return result;
};

void SoundManager::setSfxVolume(const s8& t_vol, const s8& t_ch) {
  return this->t_engine->audio.adpcm.setVolume(t_vol, t_ch);
};

const s8 SoundManager::getAvailableChannel() {
  currentAdpcmChannel = currentAdpcmChannel % MAX_ADPCM_CH;
  return currentAdpcmChannel++;
};
