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
   * @param facesMap 12 length array pair of integers col, row
   * 
   * @param isBreakable bool can be broken
   * @param isSolid bool is a solid block
   *
   */
  BlockInfo(const Blocks& type, const u8& isSingle,
            const std::initializer_list<u8>& facesMap,
            const bool& isTransparent, const bool& isBreakable = true,
            const bool& isSolid = false) {
    blockId = (u8)type;
    _isSingle = isSingle;
    _isBreakable = isBreakable;
    _isSolid = isSolid;
    _isTransparent = isTransparent;

    u8 i = 0;
    for (auto uv : facesMap) {
      _facesMap[i] = uv;
      i++;
    }
  };

  ~BlockInfo(){};

  std::array<u8, 12> _facesMap = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  u8 _isSingle;
  u8 blockId;
  bool _isBreakable;
  bool _isSolid;
  bool _isTransparent;
};