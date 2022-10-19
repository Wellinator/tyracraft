#pragma once
#include <tamtypes.h>
#include "tyra"
#include "entities/sfx_library.hpp"

using Tyra::Audio;
using Tyra::AdpcmResult;
using Tyra::Engine;

class SoundManager {
 public:
  SoundManager(Engine* t_engine);
  ~SoundManager();

  SfxLibraryCategory* getCategory(SoundFxCategory idCategory) {
    return this->soundLibrary->getCategory(idCategory);
  }

  SfxLibrarySound* getSound(SoundFxCategory idCategory, SoundFX idSound) {
    return this->soundLibrary->getSound(idCategory, idSound);
  }

  AdpcmResult playSfx(SoundFxCategory idCategory, SoundFX idSound);
  AdpcmResult playSfx(SoundFxCategory idCategory, SoundFX idSound,
                      const s8& t_ch);

 private:
  SfxLibrary* soundLibrary;
  Engine* t_engine;
};
