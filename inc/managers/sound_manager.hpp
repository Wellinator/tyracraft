#pragma once
#include <tamtypes.h>
#include "tyra"
#include "entities/sfx_library.hpp"

using Tyra::Engine;

class SoundManager {
 public:
  SoundManager(Engine* t_engine);
  ~SoundManager();

 private:
  SfxLibrary* soundLibrary;
};
