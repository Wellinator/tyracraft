#pragma once

#include "constants.hpp"
#include "models/sfx_config_model.hpp"
#include "tyra"

using Tyra::Audio;
using Tyra::Math;

class SfxConfig {
 public:
  SfxConfig();
  ~SfxConfig();

  static SfxConfigModel* getPlaceSoundConfig(Blocks blockType);
  static SfxConfigModel* getStepSoundConfig(Blocks blockType);
  static SfxConfigModel* getBreakingSoundConfig(Blocks blockType);
  static SfxConfigModel* getBrokenSoundConfig(Blocks blockType);
};
