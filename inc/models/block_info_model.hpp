#pragma once
#include <tyra>
#include <tamtypes.h>
#include <array>

class BlockInfo {
 public:
  /**
   * @brief Initialize BlockInfo
   * @param type Blocks enum block type
   * @param isSingle bool apply the same texture in all faces
   *
   * Order: Front, Bacck, Left, Right, Top, Bottom
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

    _isSingle = isSingle;
    _isBreakable = isBreakable;
    _isTransparent = isTransparent;
    _isCollidable = isCollidable;
    _isCrossed = isCrossed;

    _hardness = hardness;

    u8 i = 0;
    for (auto uv : facesMap) {
      _facesMap[i] = uv;
      i++;
    }
  };
  BlockInfo(){};

  ~BlockInfo(){};

  std::array<u8, 6> _facesMap = {0, 0, 0, 0, 0, 0};

  u8 blockId;

  u8 _isSingle;
  u8 _isBreakable;
  u8 _isCollidable;
  u8 _isTransparent;
  u8 _isCrossed;

  float _hardness = 0;
};