#pragma once
#include <tamtypes.h>
#include "tyra"

using Tyra::Sprite;

class FontChar {
 public:
  FontChar(const u8& charCode, Sprite* sprite) {
    _charCode = charCode;
    t_sprite = sprite;
  };

  ~FontChar() { delete this->t_sprite; };

  u8 _charCode;
  Sprite* t_sprite;
};
