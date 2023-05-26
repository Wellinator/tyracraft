#pragma once

#include "entities/sfx_library_category.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;

class SfxLiquidCategory : public SfxLibraryCategory {
 public:
  SfxLiquidCategory(Audio* t_audio);
  ~SfxLiquidCategory();

  void loadSounds();
};
