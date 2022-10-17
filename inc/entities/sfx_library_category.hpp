#pragma once

#include "entities/sfx_library_sound.hpp"
#include "contants.hpp"
#include <vector>
#include <tyra>

using Tyra::Audio;

enum class SoundFxCategory { Random };

class SfxLibraryCategory {
 public:
  SfxLibraryCategory(SoundFxCategory _id) { id = _id; };
  virtual ~SfxLibraryCategory(){};
  virtual void loadSounds() = 0;

  SoundFxCategory id;
  std::vector<SfxLibrarySound*> sounds;
  Audio* t_audio;
};
