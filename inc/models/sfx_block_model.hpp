#pragma once
#include <tamtypes.h>
#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"

using Tyra::Audio;

class SfxBlockModel {
 public:
  SfxBlockModel(u8 blockType, SoundFxCategory category, SoundFX sound) {
    this->_blockType = blockType;
    this->category = category;
    this->sound = sound;
  };

  ~SfxBlockModel(){};

  const u8 getType() { return this->_blockType; };

  SoundFxCategory category;
  SoundFX sound = SoundFX::None;

 private:
  u8 _blockType;
};
