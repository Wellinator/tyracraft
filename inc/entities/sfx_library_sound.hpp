#pragma once

#include "entities/sfx_library_category.hpp"
#include <tamtypes.h>
#include <tyra>

enum class SoundFX {
  // Random
  Click
};

class SfxLibrarySound {
 public:
  SfxLibrarySound(SoundFX _id) { id = _id; };
  ~SfxLibrarySound() { delete _sound; };

  audsrv_adpcm_t* _sound;
  SoundFX id;
  u8 isLoaded;
  u8 unloadAfterPlay;
};
