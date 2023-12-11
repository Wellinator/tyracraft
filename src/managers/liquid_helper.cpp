#include "managers/liquid_helper.hpp"

LiquidQuadMapModel LiquidHelper_getQuadMap(LevelMap* t_terrain,
                                           BlockOrientation orientation,
                                           Vec4* offset, u8 liquid_type) {
  LiquidQuadMapModel result;

  const u8 currentLevel =
      GetLiquidDataFromMap(t_terrain, offset->x, offset->y, offset->z);

  if (currentLevel == (u8)LiquidLevel::Percent100) {
    result.NW = (1.0F - 1.0F) * DUBLE_BLOCK_SIZE;
    result.NE = (1.0F - 1.0F) * DUBLE_BLOCK_SIZE;
    result.SE = (1.0F - 1.0F) * DUBLE_BLOCK_SIZE;
    result.SW = (1.0F - 1.0F) * DUBLE_BLOCK_SIZE;
    return result;
  }

  u8 left = 0;
  u8 topLeft = 0;
  u8 top = 0;
  u8 topRight = 0;
  u8 right = 0;
  u8 bottomRight = 0;
  u8 bottom = 0;
  u8 bottomLeft = 0;

  u8 NW = 0;
  u8 NE = 0;
  u8 SE = 0;
  u8 SW = 0;

  // Left
  if (BoundCheckMap(t_terrain, offset->x + 1, offset->y, offset->z) &&
      GetBlockFromMap(t_terrain, offset->x + 1, offset->y, offset->z) ==
          liquid_type) {
    left = GetLiquidDataFromMap(t_terrain, offset->x + 1, offset->y, offset->z);
  }

  // Top Left
  if (BoundCheckMap(t_terrain, offset->x + 1, offset->y, offset->z + 1) &&
      GetBlockFromMap(t_terrain, offset->x + 1, offset->y, offset->z + 1) ==
          liquid_type) {
    topLeft = GetLiquidDataFromMap(t_terrain, offset->x + 1, offset->y,
                                   offset->z + 1);
  }

  // Top
  if (BoundCheckMap(t_terrain, offset->x, offset->y, offset->z + 1) &&
      GetBlockFromMap(t_terrain, offset->x, offset->y, offset->z + 1) ==
          liquid_type) {
    top = GetLiquidDataFromMap(t_terrain, offset->x, offset->y, offset->z + 1);
  }

  // Top right
  if (BoundCheckMap(t_terrain, offset->x - 1, offset->y, offset->z + 1) &&
      GetBlockFromMap(t_terrain, offset->x - 1, offset->y, offset->z + 1) ==
          liquid_type) {
    topRight = GetLiquidDataFromMap(t_terrain, offset->x - 1, offset->y,
                                    offset->z + 1);
  }

  // Right
  if (BoundCheckMap(t_terrain, offset->x - 1, offset->y, offset->z) &&
      GetBlockFromMap(t_terrain, offset->x - 1, offset->y, offset->z) ==
          liquid_type) {
    right =
        GetLiquidDataFromMap(t_terrain, offset->x - 1, offset->y, offset->z);
  }

  // Bottom right
  if (BoundCheckMap(t_terrain, offset->x - 1, offset->y, offset->z - 1) &&
      GetBlockFromMap(t_terrain, offset->x - 1, offset->y, offset->z - 1) ==
          liquid_type) {
    bottomRight = GetLiquidDataFromMap(t_terrain, offset->x - 1, offset->y,
                                       offset->z - 1);
  }

  // Bottom
  if (BoundCheckMap(t_terrain, offset->x, offset->y, offset->z - 1) &&
      GetBlockFromMap(t_terrain, offset->x, offset->y, offset->z - 1) ==
          liquid_type) {
    bottom =
        GetLiquidDataFromMap(t_terrain, offset->x, offset->y, offset->z - 1);
  }

  // bottom Left
  if (BoundCheckMap(t_terrain, offset->x + 1, offset->y, offset->z - 1) &&
      GetBlockFromMap(t_terrain, offset->x + 1, offset->y, offset->z - 1) ==
          liquid_type) {
    bottomLeft = GetLiquidDataFromMap(t_terrain, offset->x + 1, offset->y,
                                      offset->z - 1);
  }

  const u8 isLinear = top == bottom || left == right;

  if (isLinear) {
    u8 isFlowToTheTop = false;
    u8 isFlowToTheRight = false;
    u8 isFlowToTheBottom = false;
    u8 isFlowToTheLeft = false;

    if (top == bottom) {
      if (left > right)
        isFlowToTheRight = true;
      else
        isFlowToTheLeft = true;
    } else {
      if (top > bottom)
        isFlowToTheBottom = true;
      else
        isFlowToTheTop = true;
    }

    if (isFlowToTheTop) {
      NW = currentLevel;
      NE = currentLevel;
      SE = bottom;
      SW = bottom;
    } else if (isFlowToTheRight) {
      NW = left;
      NE = currentLevel;
      SE = currentLevel;
      SW = left;
    } else if (isFlowToTheBottom) {
      NW = top;
      NE = top;
      SE = currentLevel;
      SW = currentLevel;
    } else if (isFlowToTheLeft) {
      NW = currentLevel;
      NE = right;
      SE = right;
      SW = currentLevel;
    }
  } else {
    u8 isFlowToTheTopRight = false;
    u8 isFlowToTheBottomRight = false;
    u8 isFlowToTheBottomLeft = false;
    u8 isFlowToTheTopLeft = false;

    if (top == right) {
      if (topRight <= top)
        isFlowToTheTopRight = true;
      else
        isFlowToTheBottomLeft = true;
    } else {
      if (topLeft <= top)
        isFlowToTheTopLeft = true;
      else
        isFlowToTheBottomRight = true;
    }

    if (isFlowToTheTopRight) {
      NW = left;
      NE = currentLevel;
      SE = bottom;
      SW = bottomLeft;
    } else if (isFlowToTheBottomRight) {
      NW = topLeft;
      NE = top;
      SE = currentLevel;
      SW = left;
    } else if (isFlowToTheBottomLeft) {
      NW = top;
      NE = topRight;
      SE = right;
      SW = currentLevel;
    } else if (isFlowToTheTopLeft) {
      NW = currentLevel;
      NE = right;
      SE = bottomRight;
      SW = bottom;
    }
  }

  switch (orientation) {
    case BlockOrientation::South:
      // Will be rotated by 90deg
      // Left turns Back & Right turns Front
      if (liquid_type == (u8)Blocks::WATER_BLOCK) {
        result.NW = LiquidHelper_getWaterHeightByVolume(NE);
        result.NE = LiquidHelper_getWaterHeightByVolume(SE);
        result.SE = LiquidHelper_getWaterHeightByVolume(SW);
        result.SW = LiquidHelper_getWaterHeightByVolume(NW);

      } else {
        result.NW = LiquidHelper_getLavaHeightByVolume(NE);
        result.NE = LiquidHelper_getLavaHeightByVolume(SE);
        result.SE = LiquidHelper_getLavaHeightByVolume(SW);
        result.SW = LiquidHelper_getLavaHeightByVolume(NW);
      }

      break;
    case BlockOrientation::West:
      // Will be rotated by 180deg
      // Left turns Right & Front turns Back
      if (liquid_type == (u8)Blocks::WATER_BLOCK) {
        result.NW = LiquidHelper_getWaterHeightByVolume(SE);
        result.NE = LiquidHelper_getWaterHeightByVolume(SW);
        result.SE = LiquidHelper_getWaterHeightByVolume(NW);
        result.SW = LiquidHelper_getWaterHeightByVolume(NE);
      } else {
        result.NW = LiquidHelper_getLavaHeightByVolume(SE);
        result.NE = LiquidHelper_getLavaHeightByVolume(SW);
        result.SE = LiquidHelper_getLavaHeightByVolume(NW);
        result.SW = LiquidHelper_getLavaHeightByVolume(NE);
      }
      break;
    case BlockOrientation::North:
      // Will be rotated by 270deg
      // Left turns Front & Right turns Back
      if (liquid_type == (u8)Blocks::WATER_BLOCK) {
        result.NW = LiquidHelper_getWaterHeightByVolume(SW);
        result.NE = LiquidHelper_getWaterHeightByVolume(NW);
        result.SE = LiquidHelper_getWaterHeightByVolume(NE);
        result.SW = LiquidHelper_getWaterHeightByVolume(SE);
      } else {
        result.NW = LiquidHelper_getLavaHeightByVolume(SW);
        result.NE = LiquidHelper_getLavaHeightByVolume(NW);
        result.SE = LiquidHelper_getLavaHeightByVolume(NE);
        result.SW = LiquidHelper_getLavaHeightByVolume(SE);
      }
      break;
    case BlockOrientation::East:
    default:
      if (liquid_type == (u8)Blocks::WATER_BLOCK) {
        result.NW = LiquidHelper_getWaterHeightByVolume(NW);
        result.NE = LiquidHelper_getWaterHeightByVolume(NE);
        result.SE = LiquidHelper_getWaterHeightByVolume(SE);
        result.SW = LiquidHelper_getWaterHeightByVolume(SW);
      } else {
        result.NW = LiquidHelper_getLavaHeightByVolume(NW);
        result.NE = LiquidHelper_getLavaHeightByVolume(NE);
        result.SE = LiquidHelper_getLavaHeightByVolume(SE);
        result.SW = LiquidHelper_getLavaHeightByVolume(SW);
      }
      break;
  }

  // if (liquid_type == (u8)Blocks::WATER_BLOCK) {
  //   result.NW = LiquidHelper_getWaterHeightByVolume(NW);
  //   result.NE = LiquidHelper_getWaterHeightByVolume(NE);
  //   result.SE = LiquidHelper_getWaterHeightByVolume(SE);
  //   result.SW = LiquidHelper_getWaterHeightByVolume(SW);
  // } else {
  //   result.NW = LiquidHelper_getLavaHeightByVolume(NW);
  //   result.NE = LiquidHelper_getLavaHeightByVolume(NE);
  //   result.SE = LiquidHelper_getLavaHeightByVolume(SE);
  //   result.SW = LiquidHelper_getLavaHeightByVolume(SW);
  // }

  return result;
}

float LiquidHelper_getWaterHeightByVolume(u8 liquid_volume) {
  switch (liquid_volume) {
    case (u8)LiquidLevel::Percent100:
      return (1.0F - 1.0F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent75:
      return (1.0F - 0.75F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent62:
      return (1.0F - 0.625F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent50:
      return (1.0F - 0.50F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent37:
      return (1.0F - 0.375F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent25:
      return (1.0F - 0.25F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent12:
      return (1.0F - 0.125F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent0:
      return DUBLE_BLOCK_SIZE;

    default:
      TYRA_WARN("WARNING! got invalid water level: ", liquid_volume);
      return DUBLE_BLOCK_SIZE;
  }
}

float LiquidHelper_getLavaHeightByVolume(u8 liquid_volume) {
  switch (liquid_volume) {
    case (u8)LiquidLevel::Percent100:
      return (1.0F - 1.0F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent75:
      return (1.0F - 0.75F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent50:
      return (1.0F - 0.50F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent25:
      return (1.0F - 0.25F) * DUBLE_BLOCK_SIZE;
    case (u8)LiquidLevel::Percent0:
      return (1.0F - 0.125F) * DUBLE_BLOCK_SIZE;

    default:
      TYRA_WARN("WARNING! got invalid lava level: ", liquid_volume);
      return DUBLE_BLOCK_SIZE;
  }
}