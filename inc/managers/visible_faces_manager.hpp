#pragma once
#include <tamtypes.h>
#include "tyra"
#include "singleton.hpp"
#include "constants.hpp"
#include "entities/level.hpp"
#include "./block_manager.hpp"
#include "managers/block/StaticBlockRepository.hpp"

class VisibleFacesManager : public Singleton<VisibleFacesManager> {
 public:
  VisibleFacesManager();
  ~VisibleFacesManager();

  u8 getVisibleFacesByOffset(const Vec4& offset);

 private:
  u8 getLiquidBlockVisibleFaces(const uint16_t x, const uint16_t y,
                                const uint16_t z, const LevelMap* map);
  u8 getSlabVisibleFaces(const uint16_t x, const uint16_t y,
                         const uint16_t z, Level* pLevel,
                         const LevelMap* map);
  u8 getLeavesVisibleFaces(const uint16_t x, const uint16_t y,
                           const uint16_t z, const LevelMap* map);
  u8 getDefaultVisibleFaces(const uint16_t x, const uint16_t y,
                            const uint16_t z, Level* pLevel,
                            const LevelMap* map);

  // -----------------------------------------------------------------------
  // Fast O(1) transparency test — uses the pre-built lookup table in
  // StaticBlockRepository. No singleton chain, no virtual dispatch.
  // Returns 0 if the position is out-of-bounds (VOID behaviour).
  // -----------------------------------------------------------------------
  inline u8 isTransparentAt(const uint16_t x, const uint16_t y,
                             const uint16_t z, const LevelMap* map) const {
    // Bounds check inlined to avoid function-call overhead.
    if (x >= map->width || y >= map->height || z >= map->length) return 0;
    const u8 blk = Level::getInstance()->GetBlockFromMap(x, y, z);
    return StaticBlockRepository::s_transparencyTable[blk];
  }

  // Directional helpers — resolve (x,y,z) once in the caller and call these.
  inline u8 isTopVisible   (uint16_t x, uint16_t y, uint16_t z, const LevelMap* m) const { return isTransparentAt(x, y + 1, z, m); }
  inline u8 isBottomVisible(uint16_t x, uint16_t y, uint16_t z, const LevelMap* m) const { return isTransparentAt(x, y - 1, z, m); }
  inline u8 isFrontVisible (uint16_t x, uint16_t y, uint16_t z, const LevelMap* m) const { return isTransparentAt(x, y, z - 1, m); }
  inline u8 isBackVisible  (uint16_t x, uint16_t y, uint16_t z, const LevelMap* m) const { return isTransparentAt(x, y, z + 1, m); }
  inline u8 isLeftVisible  (uint16_t x, uint16_t y, uint16_t z, const LevelMap* m) const { return isTransparentAt(x + 1, y, z, m); }
  inline u8 isRightVisible (uint16_t x, uint16_t y, uint16_t z, const LevelMap* m) const { return isTransparentAt(x - 1, y, z, m); }
};
