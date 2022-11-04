#pragma once
#include <tamtypes.h>

class BlockInfo {
 public:
  BlockInfo(u8 type, u8 isSingle, const float& texOffssetX,
            const float& texOffssetY, const bool& isBreakable = true,
            const bool& isSolid = false) {
    _texOffssetX = texOffssetX;
    _texOffssetY = texOffssetY;
    blockId = type;
    _isSingle = isSingle;
    _isBreakable = isBreakable;
    _isSolid = isSolid;
  };

  ~BlockInfo(){};

  float _texOffssetX;
  float _texOffssetY;
  u8 _isSingle;
  u8 blockId;
  bool _isBreakable;
  bool _isSolid;
};