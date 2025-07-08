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

    // Pack boolean flags into a single byte using bitwise operations
    flags = 0;
    if (isBreakable) flags |= FLAG_BREAKABLE;
    if (isTransparent) flags |= FLAG_TRANSPARENT;
    if (isCollidable) flags |= FLAG_COLLIDABLE;
    if (isCrossed) flags |= FLAG_CROSSED;

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

  // Bit flags for block properties (saves 3 bytes compared to 4 separate u8 fields)
  static constexpr u8 FLAG_BREAKABLE = 0x01;    // bit 0
  static constexpr u8 FLAG_TRANSPARENT = 0x02;  // bit 1
  static constexpr u8 FLAG_COLLIDABLE = 0x04;   // bit 2
  static constexpr u8 FLAG_CROSSED = 0x08;      // bit 3

  std::array<u8, 6> _facesMap = {0, 0, 0, 0, 0, 0};

  u8 blockId;
  u8 flags;  // Packed boolean flags (replaces 4 separate u8 fields)

  float _hardness = 0;

  // Accessor methods for packed flags
  inline bool isBreakable() const { return (flags & FLAG_BREAKABLE) != 0; }
  inline bool isTransparent() const { return (flags & FLAG_TRANSPARENT) != 0; }
  inline bool isCollidable() const { return (flags & FLAG_COLLIDABLE) != 0; }
  inline bool isCrossed() const { return (flags & FLAG_CROSSED) != 0; }

  // Legacy accessors for backward compatibility
  inline u8 _isBreakable() const { return isBreakable() ? 1 : 0; }
  inline u8 _isTransparent() const { return isTransparent() ? 1 : 0; }
  inline u8 _isCollidable() const { return isCollidable() ? 1 : 0; }
  inline u8 _isCrossed() const { return isCrossed() ? 1 : 0; }
};