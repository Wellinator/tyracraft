#pragma once
#include "entities/sfx_library_sound.hpp"
#include "contants.hpp"
#include <vector>
#include <tyra>

using Tyra::Audio;

enum class SoundFxCategory { Random, Dig, Step };

class SfxLibraryCategory {
 public:
  SfxLibraryCategory(SoundFxCategory _id) { id = _id; };
  virtual ~SfxLibraryCategory(){};

  virtual void loadSounds() = 0;

  SfxLibrarySound* getSound(SoundFX idSound) {
    for (size_t i = 0; i < sounds.size(); i++)
      if (sounds[i]->id == idSound) return sounds[i];

    TYRA_ASSERT(false, "Sound not found in sound fx library");
    return nullptr;
  }

  SoundFxCategory id;
  std::vector<SfxLibrarySound*> sounds;
  Audio* t_audio;
};
