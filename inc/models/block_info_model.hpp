#pragma once
#include <tamtypes.h>

class BlockInfo {
 public:
  BlockInfo(const Blocks& type, const u8 isSingle, const float& texOffssetX,
            const float& texOffssetY, const bool& isBreakable = true,
            const bool& isSolid = false)
      : _texOffssetX(texOffssetX), _texOffssetY(texOffssetY) {
    blockId = (u8)type;
    _isSingle = isSingle;
    _isBreakable = isBreakable;
    _isSolid = isSolid;
  };

  ~BlockInfo(){};

  const float _texOffssetX;
  const float _texOffssetY;
  u8 _isSingle;
  u8 blockId;
  bool _isBreakable;
  bool _isSolid;
};