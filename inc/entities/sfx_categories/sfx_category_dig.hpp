#pragma once

#include "entities/sfx_library_category.hpp"
#include "contants.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;

class SfxDigCategory : public SfxLibraryCategory {
 public:
  SfxDigCategory(Audio* t_audio);
  ~SfxDigCategory();

  void loadSounds();
};
