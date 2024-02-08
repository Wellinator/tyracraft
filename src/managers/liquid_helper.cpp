#include "managers/liquid_helper.hpp"

LiquidQuadMapModel LiquidHelper_getQuadMap(LevelMap* t_terrain,
                                           LiquidOrientation orientation,
                                           Vec4* offset, u8 liquid_type) {
  LiquidQuadMapModel result;

  const u8 currentLevel =
      GetLiquidDataFromMap(t_terrain, offset->x, offset->y, offset->z);

  if (currentLevel == (u8)LiquidLevel::Percent100) {
    result.NW = 0;
    result.NE = 0;
    result.SE = 0;
    result.SW = 0;
    return result;
  }

  u8 left = currentLevel;
  u8 topLeft = currentLevel;
  u8 top = currentLevel;
  u8 topRight = currentLevel;
  u8 right = currentLevel;
  u8 bottomRight = currentLevel;
  u8 bottom = currentLevel;
  u8 bottomLeft = currentLevel;

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

  NW = std::max({currentLevel, left, topLeft, top});
  NE = std::max({currentLevel, top, topRight, right});
  SE = std::max({currentLevel, right, bottomRight, bottom});
  SW = std::max({currentLevel, bottom, bottomLeft, left});

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

  return result;
}

float LiquidHelper_getWaterHeightByVolume(u8 liquid_volume) {
  switch (liquid_volume) {
    case (u8)LiquidLevel::Percent100:
      return DUBLE_BLOCK_SIZE - (1.0F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent75:
      return DUBLE_BLOCK_SIZE - (0.75F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent62:
      return DUBLE_BLOCK_SIZE - (0.625F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent50:
      return DUBLE_BLOCK_SIZE - (0.50F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent37:
      return DUBLE_BLOCK_SIZE - (0.375F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent25:
      return DUBLE_BLOCK_SIZE - (0.25F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent12:
      return DUBLE_BLOCK_SIZE - (0.125F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent0:
      return 0;

    default:
      TYRA_WARN("WARNING! got invalid water level: ", liquid_volume);
      return BLOCK_SIZE;
  }
}

float LiquidHelper_getLavaHeightByVolume(u8 liquid_volume) {
  switch (liquid_volume) {
    case (u8)LiquidLevel::Percent100:
      return DUBLE_BLOCK_SIZE - (1.0F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent75:
      return DUBLE_BLOCK_SIZE - (0.75F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent50:
      return DUBLE_BLOCK_SIZE - (0.50F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent25:
      return DUBLE_BLOCK_SIZE - (0.25F * DUBLE_BLOCK_SIZE);
    case (u8)LiquidLevel::Percent0:
      return DUBLE_BLOCK_SIZE - (0.125F * DUBLE_BLOCK_SIZE);

    default:
      TYRA_WARN("WARNING! got invalid lava level: ", liquid_volume);
      return DUBLE_BLOCK_SIZE;
  }
}

const u8 LiquidHelper_getMaxCorner(const u8 cornerA, const u8 cornerB,
                                   const u8 cornerC) {
  return cornerA > cornerB ? (cornerA > cornerC ? cornerA : cornerC)
                           : (cornerB > cornerC ? cornerB : cornerC);
};

const u8 LiquidHelper_getMinCorner(const u8 cornerA, const u8 cornerB,
                                   const u8 cornerC) {
  return cornerA < cornerB ? (cornerA < cornerC ? cornerA : cornerC)
                           : (cornerB < cornerC ? cornerB : cornerC);
};