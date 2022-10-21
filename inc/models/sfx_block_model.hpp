#pragma once
#include <tamtypes.h>
#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"

using Tyra::Audio;

class SfxBlockModel {
 public:
  SfxBlockModel(u8 blockType, SoundFxCategory category, SoundFX sfx) {
    this->_blockType = blockType;
    this->category = category;
    this->sfx = sfx;
  };

  ~SfxBlockModel(){};

  const u8 getType() { return this->_blockType; };

  SoundFxCategory category;
  SoundFX sfx = SoundFX::None;

 private:
  u8 _blockType;
};
