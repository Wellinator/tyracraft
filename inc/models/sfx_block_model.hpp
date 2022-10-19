#pragma once
#include <tamtypes.h>
#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"

using Tyra::Audio;

class SfxBlockModel {
 public:
  SfxBlockModel(u8 blockType, SoundFxCategory category, SoundFX onPlacement,
                SoundFX onBreaking, SoundFX onDestroy, SoundFX onStep) {
    this->_blockType = blockType;
    this->category = category;
    this->onPlacement = onPlacement;
    this->onBreaking = onBreaking;
    this->onDestroy = onDestroy;
    this->onStep = onStep;
  };

  ~SfxBlockModel(){};

  const u8 getType() { return this->_blockType; };

  SoundFxCategory category;
  SoundFX onPlacement = SoundFX::None;
  SoundFX onBreaking = SoundFX::None;
  SoundFX onDestroy = SoundFX::None;
  SoundFX onStep = SoundFX::None;

 private:
  u8 _blockType;
};
