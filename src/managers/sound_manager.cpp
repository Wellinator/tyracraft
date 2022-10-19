#include "managers/sound_manager.hpp"
#include <tyra>

SoundManager::SoundManager(Engine* t_engine) {
  this->t_engine = t_engine;
  this->soundLibrary = new SfxLibrary(&t_engine->audio);
}

SoundManager::~SoundManager() { delete soundLibrary; }

AdpcmResult SoundManager::playSfx(SoundFxCategory idCategory, SoundFX idSound) {
  return this->playSfx(idCategory, idSound, -1);
};

AdpcmResult SoundManager::playSfx(SoundFxCategory idCategory, SoundFX idSound,
                                  const s8& t_ch) {
  SfxLibrarySound* t_sound = this->getSound(idCategory, idSound);

  TYRA_ASSERT(t_sound, "Sound FX not found in sound library");
  TYRA_ASSERT(t_sound->isLoaded, "Sound FX not loaded");

  return this->t_engine->audio.adpcm.tryPlay(t_sound->_sound, t_ch);
};
