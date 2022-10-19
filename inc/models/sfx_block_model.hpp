#pragma once
#include <tamtypes.h>
#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"

struct SfxBlockModel {
 public:
  SfxBlockModel(SoundFxCategory category) { this->_category = category; };
  ~SfxBlockModel(){};

  const SoundFxCategory getCategory() { return this->_category; };

  SoundFX onPlacement = SoundFX::None;
  SoundFX onBreaking = SoundFX::None;
  SoundFX onDestroy = SoundFX::None;

 private:
  SoundFxCategory _category;
};
