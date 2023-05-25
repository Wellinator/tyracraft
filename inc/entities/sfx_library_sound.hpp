#pragma once

#include "entities/sfx_library_category.hpp"
#include <tamtypes.h>
#include <tyra>
#include <audsrv.h>

enum class SoundFX {
  None,

  // Random
  Click,
  WoodClick,

  // Dig
  Grass1,
  Gravel1,
  Sand1,
  Snow1,
  Stone1,
  Wood1
};

class SfxLibrarySound {
 public:
  SfxLibrarySound(SoundFX _id) { id = _id; };
  ~SfxLibrarySound() { delete _sound; };

  audsrv_adpcm_t* _sound;
  SoundFX id;
};
