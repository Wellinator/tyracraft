#include "managers/sound_manager.hpp"

SoundManager::SoundManager(Engine* t_engine) {
  this->soundLibrary = new SfxLibrary(&t_engine->audio);
}

SoundManager::~SoundManager() { delete soundLibrary; }
