#pragma once

#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"
#include "entities/sfx_categories/sfx_category_random.hpp"
#include "entities/sfx_categories/sfx_category_dig.hpp"
#include "entities/sfx_categories/sfx_category_step.hpp"
#include <vector>
#include "tyra"

using Tyra::Audio;
using Tyra::FileUtils;

class SfxLibrary {
 public:
  SfxLibrary(Audio* t_audio);
  ~SfxLibrary();

  SfxLibraryCategory* getCategory(SoundFxCategory idCategory);
  SfxLibrarySound* getSound(SoundFxCategory idCategory, SoundFX idSound);

 private:
  Audio* t_audio;

  /** Item id */
  std::vector<SfxLibraryCategory*> categories;
  void buildSoundFXLibraries(Audio* t_audio);
};
