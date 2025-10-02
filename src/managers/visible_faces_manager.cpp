#include "managers/visible_faces_manager.hpp"
#include "entities/level.hpp"

VisibleFacesManager::VisibleFacesManager() : Singleton<VisibleFacesManager>() {}

VisibleFacesManager::~VisibleFacesManager() {}

u8 VisibleFacesManager::getVisibleFacesByOffset(const Vec4& offset) {
  u8 visibleFaces = 0;
  Level* pLevel = Level::getInstance();
  const Blocks block_type = static_cast<Blocks>(
      pLevel->SafeGetBlockFromMap(offset.x, offset.y, offset.z));

  if (block_type == Blocks::WATER_BLOCK || block_type == Blocks::LAVA_BLOCK) {
    visibleFaces = getLiquidBlockVisibleFaces(offset);
  } else if ((u8)block_type >= (u8)Blocks::STONE_SLAB &&
             (u8)block_type <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
    visibleFaces = getSlabVisibleFaces(offset);
  } else if (block_type == Blocks::OAK_LEAVES_BLOCK ||
             block_type == Blocks::BIRCH_LEAVES_BLOCK) {
    visibleFaces = getLeavesVisibleFaces(offset);
  } else {
    visibleFaces = getDefaultVisibleFaces(offset);
  }

  return visibleFaces;
}

u8 VisibleFacesManager::getLiquidBlockVisibleFaces(const Vec4& offset) {
  u8 result = 0b000000;

  const auto x = offset.x;
  const auto y = offset.y;
  const auto z = offset.z;

  Level* pLevel = Level::getInstance();
  const u8 currentLevel = pLevel->GetLiquidDataFromMap(x, y, z);

  const Blocks bFront =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y, z - 1));
  const Blocks bBack =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y, z + 1));
  const Blocks bRight =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x - 1, y, z));
  const Blocks bLeft =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x + 1, y, z));
  const Blocks bTop =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y + 1, z));
  const Blocks bBottom =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y - 1, z));

  BlockManager* pBlockManager = BlockManager::getInstance();

  // TODO: refactor face visibility checks for liquid blocks
  // Front
  if (bFront != Blocks::LAVA_BLOCK && bFront != Blocks::WATER_BLOCK &&
      bFront != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bFront ||
       pBlockManager->isBlockTransparent(bFront)))
    result = result | FRONT_VISIBLE;
  // Back
  if (bBack != Blocks::LAVA_BLOCK && bBack != Blocks::WATER_BLOCK &&
      bBack != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bBack || pBlockManager->isBlockTransparent(bBack)))
    result = result | BACK_VISIBLE;
  // Right
  if (bRight != Blocks::LAVA_BLOCK && bRight != Blocks::WATER_BLOCK &&
      bRight != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bRight ||
       pBlockManager->isBlockTransparent(bRight)))
    result = result | RIGHT_VISIBLE;
  // Left
  if (bLeft != Blocks::LAVA_BLOCK && bLeft != Blocks::WATER_BLOCK &&
      bLeft != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bLeft || pBlockManager->isBlockTransparent(bLeft)))
    result = result | LEFT_VISIBLE;

  // Top
  if (bTop != Blocks::LAVA_BLOCK && bTop != Blocks::WATER_BLOCK &&
      bTop != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bTop || pBlockManager->isBlockTransparent(bTop) ||
       currentLevel < (u8)LiquidLevel::Percent100))
    result = result | TOP_VISIBLE;
  // Bottom
  if (bBottom != Blocks::LAVA_BLOCK && bBottom != Blocks::WATER_BLOCK &&
      bBottom != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bBottom ||
       pBlockManager->isBlockTransparent(bBottom)))
    result = result | BOTTOM_VISIBLE;

  return result;
}

u8 VisibleFacesManager::getSlabVisibleFaces(const Vec4& offset) {
  Level* pLevel = Level::getInstance();
  const BlockOrientation orientationXZ =
      pLevel->GetBlockOrientationDataFromMap(offset.x, offset.y, offset.z);
  const SlabOrientation orientationY =
      pLevel->GetSlabOrientationDataFromMap(offset.x, offset.y, offset.z);

  u8 result = 0b000000;

  switch (orientationXZ) {
    case BlockOrientation::North:
      // Will be rotated by 90deg
      // Left turns Back & Right turns Front
      if (isLeftFaceVisible(&offset)) result = result | BACK_VISIBLE;
      if (isFrontFaceVisible(&offset)) result = result | LEFT_VISIBLE;
      if (isBackFaceVisible(&offset)) result = result | RIGHT_VISIBLE;
      if (isRightFaceVisible(&offset)) result = result | FRONT_VISIBLE;
      break;
    case BlockOrientation::South:
      // Will be rotated by 270deg
      // Left turns Front & Right turns Back
      if (isLeftFaceVisible(&offset)) result = result | FRONT_VISIBLE;
      if (isFrontFaceVisible(&offset)) result = result | RIGHT_VISIBLE;
      if (isRightFaceVisible(&offset)) result = result | BACK_VISIBLE;
      if (isBackFaceVisible(&offset)) result = result | LEFT_VISIBLE;
      break;
    case BlockOrientation::West:
      // Will be rotated by 180deg
      // Left turns Right & Front turns Back
      if (isLeftFaceVisible(&offset)) result = result | RIGHT_VISIBLE;
      if (isFrontFaceVisible(&offset)) result = result | BACK_VISIBLE;
      if (isBackFaceVisible(&offset)) result = result | FRONT_VISIBLE;
      if (isRightFaceVisible(&offset)) result = result | LEFT_VISIBLE;
      break;
    case BlockOrientation::East:
    default:
      if (isFrontFaceVisible(&offset)) result = result | FRONT_VISIBLE;
      if (isBackFaceVisible(&offset)) result = result | BACK_VISIBLE;
      if (isRightFaceVisible(&offset)) result = result | RIGHT_VISIBLE;
      if (isLeftFaceVisible(&offset)) result = result | LEFT_VISIBLE;
      break;
  }

  if (orientationY == SlabOrientation::Top) {
    if (isTopFaceVisible(&offset)) result = result | TOP_VISIBLE;
    result = result | BOTTOM_VISIBLE;
  } else if (orientationY == SlabOrientation::Bottom) {
    result = result | TOP_VISIBLE;
    if (isBottomFaceVisible(&offset)) result = result | BOTTOM_VISIBLE;
  }

  return result;
}

u8 VisibleFacesManager::getLeavesVisibleFaces(const Vec4& offset) {
  u8 result = 0b000000;
  Level* pLevel = Level::getInstance();

  const uint16_t x = offset.x;
  const uint16_t y = offset.y;
  const uint16_t z = offset.z;

  const Blocks bFront =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y, z - 1));
  const Blocks bBlack =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y, z + 1));
  const Blocks bRight =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x - 1, y, z));
  const Blocks bLeft =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x + 1, y, z));
  const Blocks bTop =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y + 1, z));
  const Blocks bBottom =
      static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y - 1, z));

  if (Blocks::AIR_BLOCK == bFront || Blocks::OAK_LOG_BLOCK == bFront ||
      Blocks::BIRCH_LOG_BLOCK == bFront)
    result = result | FRONT_VISIBLE;
  if (Blocks::AIR_BLOCK == bBlack || Blocks::OAK_LOG_BLOCK == bBlack ||
      Blocks::BIRCH_LOG_BLOCK == bBlack)
    result = result | BACK_VISIBLE;
  if (Blocks::AIR_BLOCK == bRight || Blocks::OAK_LOG_BLOCK == bRight ||
      Blocks::BIRCH_LOG_BLOCK == bRight)
    result = result | RIGHT_VISIBLE;
  if (Blocks::AIR_BLOCK == bLeft || Blocks::OAK_LOG_BLOCK == bLeft ||
      Blocks::BIRCH_LOG_BLOCK == bLeft)
    result = result | LEFT_VISIBLE;
  if (Blocks::AIR_BLOCK == bTop || Blocks::OAK_LOG_BLOCK == bTop ||
      Blocks::BIRCH_LOG_BLOCK == bTop)
    result = result | TOP_VISIBLE;
  if (Blocks::AIR_BLOCK == bBottom || Blocks::OAK_LOG_BLOCK == bBottom ||
      Blocks::BIRCH_LOG_BLOCK == bBottom)
    result = result | BOTTOM_VISIBLE;

  return result;
}

u8 VisibleFacesManager::getDefaultVisibleFaces(const Vec4& offset) {
  Level* pLevel = Level::getInstance();
  const BlockOrientation orientation =
      pLevel->GetBlockOrientationDataFromMap(offset.x, offset.y, offset.z);

  u8 result = 0b000000;

  switch (orientation) {
    case BlockOrientation::North:
      // Will be rotated by 90deg
      // Left turns Back & Right turns Front
      if (isLeftFaceVisible(&offset)) result |= BACK_VISIBLE;
      if (isFrontFaceVisible(&offset)) result |= LEFT_VISIBLE;
      if (isBackFaceVisible(&offset)) result |= RIGHT_VISIBLE;
      if (isRightFaceVisible(&offset)) result |= FRONT_VISIBLE;
      break;
    case BlockOrientation::South:
      // Will be rotated by 270deg
      // Left turns Front & Right turns Back
      if (isLeftFaceVisible(&offset)) result |= FRONT_VISIBLE;
      if (isFrontFaceVisible(&offset)) result |= RIGHT_VISIBLE;
      if (isRightFaceVisible(&offset)) result |= BACK_VISIBLE;
      if (isBackFaceVisible(&offset)) result |= LEFT_VISIBLE;
      break;
    case BlockOrientation::West:
      // Will be rotated by 180deg
      // Left turns Right & Front turns Back
      if (isLeftFaceVisible(&offset)) result |= RIGHT_VISIBLE;
      if (isFrontFaceVisible(&offset)) result |= BACK_VISIBLE;
      if (isBackFaceVisible(&offset)) result |= FRONT_VISIBLE;
      if (isRightFaceVisible(&offset)) result |= LEFT_VISIBLE;
      break;
    case BlockOrientation::East:
    default:
      if (isLeftFaceVisible(&offset)) result |= LEFT_VISIBLE;
      if (isFrontFaceVisible(&offset)) result |= FRONT_VISIBLE;
      if (isBackFaceVisible(&offset)) result |= BACK_VISIBLE;
      if (isRightFaceVisible(&offset)) result |= RIGHT_VISIBLE;
      break;
  }

  if (isTopFaceVisible(&offset)) result |= TOP_VISIBLE;
  if (isBottomFaceVisible(&offset)) result |= BOTTOM_VISIBLE;

  return result;
}

u8 VisibleFacesManager::isTopFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y + 1,
                                      t_blockOffset->z);
}

u8 VisibleFacesManager::isBottomFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y - 1,
                                      t_blockOffset->z);
}

u8 VisibleFacesManager::isFrontFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y,
                                      t_blockOffset->z - 1);
}

u8 VisibleFacesManager::isBackFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y,
                                      t_blockOffset->z + 1);
}

u8 VisibleFacesManager::isLeftFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x + 1, t_blockOffset->y,
                                      t_blockOffset->z);
}

u8 VisibleFacesManager::isRightFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x - 1, t_blockOffset->y,
                                      t_blockOffset->z);
}

u8 VisibleFacesManager::isBlockTransparentAtPosition(const uint16_t x,
                                                     const uint16_t y,
                                                     const uint16_t z) {
  Level* pLevel = Level::getInstance();
  const Blocks blk = static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y, z));

  if (blk == Blocks::AIR_BLOCK) return 1;
  if (blk == Blocks::VOID) return 0;

  Block* blockTemplate =
      BlockManager::getInstance()->getBlockTemplateByType(blk);
  return blockTemplate->hasTransparency() || blk == Blocks::LAVA_BLOCK ||
         blk == Blocks::WATER_BLOCK;
}