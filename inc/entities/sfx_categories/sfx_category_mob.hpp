#pragma once

#include "entities/sfx_library_category.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;

class SfxMobCategory : public SfxLibraryCategory {
 public:
  SfxMobCategory(Audio* t_audio);
  ~SfxMobCategory();

  void loadSounds();
  void loadPigSounds();
};
