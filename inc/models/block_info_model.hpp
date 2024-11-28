#pragma once
#include <tyra>
#include <tamtypes.h>
#include <array>

class BlockInfo {
 public:
  /**
   * @brief Initialize BlockInfo
   * @param type Blocks enum block type
   *
   * Order: Top, Bottom, Left, Right, Back, Front
   * @param facesMap 6 length array of integers index
   *
   * @param isBreakable bool can be broken
   * @param isCollidable bool is a collidable block
   *
   */

  BlockInfo(const Blocks& type, const u8& isSingle,
            const std::initializer_list<u8>& facesMap, const float hardness,
            const bool& isTransparent, const bool& isBreakable = true,
            const bool& isCollidable = true, const bool& isCrossed = false) {
    blockId = (u8)type;

    _isBreakable = isBreakable;
    _isTransparent = isTransparent;
    _isCollidable = isCollidable;
    _isCrossed = isCrossed;

    _hardness = hardness;

    if (facesMap.size() == 1) {
      for (auto uv : facesMap) {
        _facesMap[0] = uv;
        _facesMap[1] = uv;
        _facesMap[2] = uv;
        _facesMap[3] = uv;
        _facesMap[4] = uv;
        _facesMap[5] = uv;
      }
    } else {
      u8 i = 0;
      for (auto uv : facesMap) {
        _facesMap[i] = uv;
        i++;
      }
    }
  };
  
  BlockInfo(){};

  ~BlockInfo(){};

  std::array<u8, 6> _facesMap = {0, 0, 0, 0, 0, 0};

  u8 blockId;

  u8 _isBreakable;
  u8 _isCollidable;
  u8 _isTransparent;
  u8 _isCrossed;

  float _hardness = 0;
};