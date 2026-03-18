#include "managers/visible_faces_manager.hpp"
#include "entities/level.hpp"

VisibleFacesManager::VisibleFacesManager() : Singleton<VisibleFacesManager>() {}

VisibleFacesManager::~VisibleFacesManager() {}

// ---------------------------------------------------------------------------
// Public entry point — resolves Level* once and dispatches by block type.
// ---------------------------------------------------------------------------
u8 VisibleFacesManager::getVisibleFacesByOffset(const Vec4& offset) {
  const uint16_t x = static_cast<uint16_t>(offset.x);
  const uint16_t y = static_cast<uint16_t>(offset.y);
  const uint16_t z = static_cast<uint16_t>(offset.z);

  Level* pLevel        = Level::getInstance();
  const LevelMap* map  = &pLevel->map;

    const Blocks block_type =
      static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));

  if (block_type == Blocks::WATER_BLOCK || block_type == Blocks::LAVA_BLOCK) {
    return getLiquidBlockVisibleFaces(x, y, z, map);
  }
  if ((u8)block_type >= (u8)Blocks::STONE_SLAB &&
      (u8)block_type <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
    return getSlabVisibleFaces(x, y, z, pLevel, map);
  }
  if (block_type == Blocks::OAK_LEAVES_BLOCK ||
      block_type == Blocks::BIRCH_LEAVES_BLOCK) {
    return getLeavesVisibleFaces(x, y, z, map);
  }
  return getDefaultVisibleFaces(x, y, z, pLevel, map);
}

// ---------------------------------------------------------------------------
// Liquid blocks — reads all 6 neighbours once, checks same-liquid adjacency
// and out-of-bounds (VOID) inline, then uses transparency table for the rest.
// ---------------------------------------------------------------------------
u8 VisibleFacesManager::getLiquidBlockVisibleFaces(const uint16_t x,
                                                   const uint16_t y,
                                                   const uint16_t z,
                                                   const LevelMap* map) {
  const uint32_t W = map->width;
  const uint32_t L = map->length;

  // Helper lambda: safe block read (returns VOID ID = 0 if out of bounds).
  auto safeBlock = [&](uint16_t bx, uint16_t by, uint16_t bz) -> u8 {
    if (bx >= W || by >= map->height || bz >= L) return (u8)Blocks::VOID;
    return Level::getInstance()->GetBlockFromMap(bx, by, bz);
  };

  const u8 bFront  = safeBlock(x,     y,     z - 1);
  const u8 bBack   = safeBlock(x,     y,     z + 1);
  const u8 bRight  = safeBlock(x - 1, y,     z    );
  const u8 bLeft   = safeBlock(x + 1, y,     z    );
  const u8 bTop    = safeBlock(x,     y + 1, z    );
  const u8 bBottom = safeBlock(x,     y - 1, z    );

  // Current liquid level for the top-face partial-fill check.
    const u8 currentLevel =
      Level::getInstance()->GetLiquidDataFromMap(x, y, z);

  // A liquid face is visible when the neighbour is NOT the same liquid type
  // AND is not VOID AND is transparent (air, glass, etc.).
  // We use s_transparencyTable for the final transparent check — O(1).
  const u8* T = StaticBlockRepository::s_transparencyTable;

  u8 result = 0;

  auto liquidFaceVisible = [&](u8 nb) -> bool {
    const Blocks nbt = static_cast<Blocks>(nb);
    return nbt != Blocks::LAVA_BLOCK && nbt != Blocks::WATER_BLOCK &&
           nbt != Blocks::VOID && (nbt == Blocks::AIR_BLOCK || T[nb]);
  };

  if (liquidFaceVisible(bFront )) result |= FRONT_VISIBLE;
  if (liquidFaceVisible(bBack  )) result |= BACK_VISIBLE;
  if (liquidFaceVisible(bRight )) result |= RIGHT_VISIBLE;
  if (liquidFaceVisible(bLeft  )) result |= LEFT_VISIBLE;

  // Top face: also visible when the liquid is not full.
  {
    const Blocks nbt = static_cast<Blocks>(bTop);
    if (nbt != Blocks::LAVA_BLOCK && nbt != Blocks::WATER_BLOCK &&
        nbt != Blocks::VOID &&
        (nbt == Blocks::AIR_BLOCK || T[bTop] ||
         currentLevel < (u8)LiquidLevel::Percent100))
      result |= TOP_VISIBLE;
  }
  if (liquidFaceVisible(bBottom)) result |= BOTTOM_VISIBLE;

  return result;
}

// ---------------------------------------------------------------------------
// Slab blocks — orientation remapping from getDefaultVisibleFaces.
// Level* is needed only for orientation metadata reads.
// ---------------------------------------------------------------------------
u8 VisibleFacesManager::getSlabVisibleFaces(const uint16_t x,
                                            const uint16_t y,
                                            const uint16_t z,
                                            Level* pLevel,
                                            const LevelMap* map) {
  const BlockOrientation orientationXZ =
      pLevel->GetBlockOrientationDataFromMap(x, y, z);
  const SlabOrientation orientationY =
      pLevel->GetSlabOrientationDataFromMap(x, y, z);

  u8 result = 0;

  switch (orientationXZ) {
    case BlockOrientation::North:
      // Rotated 90°: Left→Back, Front→Left, Back→Right, Right→Front
      if (isLeftVisible  (x, y, z, map)) result |= BACK_VISIBLE;
      if (isFrontVisible (x, y, z, map)) result |= LEFT_VISIBLE;
      if (isBackVisible  (x, y, z, map)) result |= RIGHT_VISIBLE;
      if (isRightVisible (x, y, z, map)) result |= FRONT_VISIBLE;
      break;
    case BlockOrientation::South:
      // Rotated 270°: Left→Front, Front→Right, Right→Back, Back→Left
      if (isLeftVisible  (x, y, z, map)) result |= FRONT_VISIBLE;
      if (isFrontVisible (x, y, z, map)) result |= RIGHT_VISIBLE;
      if (isRightVisible (x, y, z, map)) result |= BACK_VISIBLE;
      if (isBackVisible  (x, y, z, map)) result |= LEFT_VISIBLE;
      break;
    case BlockOrientation::West:
      // Rotated 180°: Left→Right, Front→Back, Back→Front, Right→Left
      if (isLeftVisible  (x, y, z, map)) result |= RIGHT_VISIBLE;
      if (isFrontVisible (x, y, z, map)) result |= BACK_VISIBLE;
      if (isBackVisible  (x, y, z, map)) result |= FRONT_VISIBLE;
      if (isRightVisible (x, y, z, map)) result |= LEFT_VISIBLE;
      break;
    case BlockOrientation::East:
    default:
      if (isFrontVisible (x, y, z, map)) result |= FRONT_VISIBLE;
      if (isBackVisible  (x, y, z, map)) result |= BACK_VISIBLE;
      if (isRightVisible (x, y, z, map)) result |= RIGHT_VISIBLE;
      if (isLeftVisible  (x, y, z, map)) result |= LEFT_VISIBLE;
      break;
  }

  if (orientationY == SlabOrientation::Top) {
    if (isTopVisible   (x, y, z, map)) result |= TOP_VISIBLE;
    result |= BOTTOM_VISIBLE;
  } else if (orientationY == SlabOrientation::Bottom) {
    result |= TOP_VISIBLE;
    if (isBottomVisible(x, y, z, map)) result |= BOTTOM_VISIBLE;
  }

  return result;
}

// ---------------------------------------------------------------------------
// Leaves — special rules: only AIR or log blocks reveal a face.
// All 6 neighbours are read in one pass to keep the map pointer hot.
// ---------------------------------------------------------------------------
u8 VisibleFacesManager::getLeavesVisibleFaces(const uint16_t x,
                                              const uint16_t y,
                                              const uint16_t z,
                                              const LevelMap* map) {
  const uint32_t W = map->width;
  const uint32_t L = map->length;

  auto safeBlock = [&](uint16_t bx, uint16_t by, uint16_t bz) -> u8 {
    if (bx >= W || by >= map->height || bz >= L) return (u8)Blocks::VOID;
    return Level::getInstance()->GetBlockFromMap(bx, by, bz);
  };

  const u8 bFront  = safeBlock(x,     y,     z - 1);
  const u8 bBack   = safeBlock(x,     y,     z + 1);
  const u8 bRight  = safeBlock(x - 1, y,     z    );
  const u8 bLeft   = safeBlock(x + 1, y,     z    );
  const u8 bTop    = safeBlock(x,     y + 1, z    );
  const u8 bBottom = safeBlock(x,     y - 1, z    );

  auto leavesVisible = [](u8 nb) -> bool {
    const Blocks nbt = static_cast<Blocks>(nb);
    return nbt == Blocks::AIR_BLOCK || nbt == Blocks::OAK_LOG_BLOCK ||
           nbt == Blocks::BIRCH_LOG_BLOCK;
  };

  u8 result = 0;
  if (leavesVisible(bFront )) result |= FRONT_VISIBLE;
  if (leavesVisible(bBack  )) result |= BACK_VISIBLE;
  if (leavesVisible(bRight )) result |= RIGHT_VISIBLE;
  if (leavesVisible(bLeft  )) result |= LEFT_VISIBLE;
  if (leavesVisible(bTop   )) result |= TOP_VISIBLE;
  if (leavesVisible(bBottom)) result |= BOTTOM_VISIBLE;

  return result;
}

// ---------------------------------------------------------------------------
// Default (standard cuboid blocks) — uses transparency table for all 6 faces.
// Level* is needed only for orientation metadata reads.
// ---------------------------------------------------------------------------
u8 VisibleFacesManager::getDefaultVisibleFaces(const uint16_t x,
                                               const uint16_t y,
                                               const uint16_t z,
                                               Level* pLevel,
                                               const LevelMap* map) {
  const BlockOrientation orientation =
      pLevel->GetBlockOrientationDataFromMap(x, y, z);

  u8 result = 0;

  switch (orientation) {
    case BlockOrientation::North:
      // Rotated 90°: Left→Back, Front→Left, Back→Right, Right→Front
      if (isLeftVisible  (x, y, z, map)) result |= BACK_VISIBLE;
      if (isFrontVisible (x, y, z, map)) result |= LEFT_VISIBLE;
      if (isBackVisible  (x, y, z, map)) result |= RIGHT_VISIBLE;
      if (isRightVisible (x, y, z, map)) result |= FRONT_VISIBLE;
      break;
    case BlockOrientation::South:
      // Rotated 270°: Left→Front, Front→Right, Right→Back, Back→Left
      if (isLeftVisible  (x, y, z, map)) result |= FRONT_VISIBLE;
      if (isFrontVisible (x, y, z, map)) result |= RIGHT_VISIBLE;
      if (isRightVisible (x, y, z, map)) result |= BACK_VISIBLE;
      if (isBackVisible  (x, y, z, map)) result |= LEFT_VISIBLE;
      break;
    case BlockOrientation::West:
      // Rotated 180°: Left→Right, Front→Back, Back→Front, Right→Left
      if (isLeftVisible  (x, y, z, map)) result |= RIGHT_VISIBLE;
      if (isFrontVisible (x, y, z, map)) result |= BACK_VISIBLE;
      if (isBackVisible  (x, y, z, map)) result |= FRONT_VISIBLE;
      if (isRightVisible (x, y, z, map)) result |= LEFT_VISIBLE;
      break;
    case BlockOrientation::East:
    default:
      if (isLeftVisible  (x, y, z, map)) result |= LEFT_VISIBLE;
      if (isFrontVisible (x, y, z, map)) result |= FRONT_VISIBLE;
      if (isBackVisible  (x, y, z, map)) result |= BACK_VISIBLE;
      if (isRightVisible (x, y, z, map)) result |= RIGHT_VISIBLE;
      break;
  }

  if (isTopVisible   (x, y, z, map)) result |= TOP_VISIBLE;
  if (isBottomVisible(x, y, z, map)) result |= BOTTOM_VISIBLE;

  return result;
}