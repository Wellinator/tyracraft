#include "managers/sound_manager.hpp"

SoundManager::SoundManager(Engine* t_engine) {
  this->t_engine = t_engine;
  this->soundLibrary = new SfxLibrary(&t_engine->audio);
}

SoundManager::~SoundManager() { delete soundLibrary; }

void SoundManager::playSfx(SoundFxCategory idCategory, SoundFX idSound) {
  SfxLibrarySound* t_sound = this->getSound(idCategory, idSound);
  this->t_engine->audio.adpcm.tryPlay(t_sound->_sound);
  this->t_engine->audio.adpcm.playWait(t_sound->_sound);
};
