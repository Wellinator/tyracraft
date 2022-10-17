#pragma once
#include <tamtypes.h>
#include "tyra"
#include "entities/sfx_library.hpp"

using Tyra::Engine;

class SoundManager {
 public:
  SoundManager(Engine* t_engine);
  ~SoundManager();

  SfxLibraryCategory* getCategory(SoundFxCategory idCategory) {
    this->soundLibrary->getCategory(idCategory);
  }

  SfxLibrarySound* getSound(SoundFxCategory idCategory, SoundFX idSound) {
    this->soundLibrary->getSound(idCategory, idSound);
  }

  void playSfx(SoundFxCategory idCategory, SoundFX idSound);

 private:
  SfxLibrary* soundLibrary;
  Engine* t_engine;
};
