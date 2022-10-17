#pragma once

#include "entities/sfx_library_category.hpp"
#include "contants.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;

class SfxRandomCategory : public SfxLibraryCategory {
 public:
  SfxRandomCategory(Audio* t_audio);
  ~SfxRandomCategory();

  void loadSounds();
};
