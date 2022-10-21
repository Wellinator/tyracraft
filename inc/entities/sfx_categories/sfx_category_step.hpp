#pragma once

#include "entities/sfx_library_category.hpp"
#include "contants.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;

class SfxStepCategory : public SfxLibraryCategory {
 public:
  SfxStepCategory(Audio* t_audio);
  ~SfxStepCategory();

  void loadSounds();
};
