#include "managers/light_manager.hpp"

LightManager::LightManager(LevelMap* t_terrain) { terrain = t_terrain; }

LightManager::~LightManager() {}

void LightManager::init() {}

void LightManager::update() {}

u8 LightManager::getVertexAO(bool side1, bool corner, bool side2) {
  if (side1 && side2) {
    return 0;
  } else if ((side1 && corner && !side2) || (!side1 && corner && side2)) {
    return 1;
  } else if (!side1 && !corner && !side2) {
    return 3;
  } else {
    return 2;
  }
}

/**
 * @brief Return 4 values of AO possibility, one for each corner
 *
 * @param blocksNeightbors
 * @return std::array<bool, 4>
 *
 * Result struct:
 * 2  3
 * 0  1
 */
std::array<u8, 4> LightManager::getCornersAOValues(
    std::array<u8, 8> blocksNeightbors) {
  auto result = std::array<u8, 4>() = {
      LightManager::getVertexAO(blocksNeightbors[6], blocksNeightbors[5],
                                blocksNeightbors[4]),
      LightManager::getVertexAO(blocksNeightbors[4], blocksNeightbors[3],
                                blocksNeightbors[2]),
      LightManager::getVertexAO(blocksNeightbors[0], blocksNeightbors[7],
                                blocksNeightbors[6]),
      LightManager::getVertexAO(blocksNeightbors[2], blocksNeightbors[1],
                                blocksNeightbors[0]),
  };
  return result;
}

float LightManager::calcAOIntensity(u8 AOValue) {
  switch (AOValue) {
    case 0:
      return 0.5F;

    case 1:
      return 0.7F;

    case 2:
      return 0.8F;

    default:
      return 1.0F;
  }
}

Color LightManager::IntensifyColor(Color* color, const float intensity) {
  return Color(color->r * intensity, color->g * intensity, color->b * intensity,
               color->a);
}

void LightManager::ApplyLightToFace(Color* baseColor, Block* targetBlock,
                                    FACE_SIDE faceSide, LevelMap* t_terrain,
                                    const float sunlightIntensity) {
  const float MAX_LIGHT_VALUE = 15.0F;
  float intensity = 0.3F;
  u8 sunLightLevel;
  u8 lightLevel;

  switch (faceSide) {
    case FACE_SIDE::TOP:
      sunLightLevel =
          GetSunLightFromMap(t_terrain, targetBlock->offset.x,
                             targetBlock->offset.y + 1, targetBlock->offset.z);
      lightLevel = GetBlockLightFromMap(t_terrain, targetBlock->offset.x,
                                        targetBlock->offset.y + 1,
                                        targetBlock->offset.z);
      break;

    case FACE_SIDE::BOTTOM:
      sunLightLevel =
          GetSunLightFromMap(t_terrain, targetBlock->offset.x,
                             targetBlock->offset.y - 1, targetBlock->offset.z);
      lightLevel = GetBlockLightFromMap(t_terrain, targetBlock->offset.x,
                                        targetBlock->offset.y - 1,
                                        targetBlock->offset.z);
      break;

    case FACE_SIDE::LEFT:
      sunLightLevel =
          GetSunLightFromMap(t_terrain, targetBlock->offset.x + 1,
                             targetBlock->offset.y, targetBlock->offset.z);
      lightLevel =
          GetBlockLightFromMap(t_terrain, targetBlock->offset.x + 1,
                               targetBlock->offset.y, targetBlock->offset.z);
      break;

    case FACE_SIDE::RIGHT:
      sunLightLevel =
          GetSunLightFromMap(t_terrain, targetBlock->offset.x - 1,
                             targetBlock->offset.y, targetBlock->offset.z);
      lightLevel =
          GetBlockLightFromMap(t_terrain, targetBlock->offset.x - 1,
                               targetBlock->offset.y, targetBlock->offset.z);
      break;

    case FACE_SIDE::BACK:
      sunLightLevel =
          GetSunLightFromMap(t_terrain, targetBlock->offset.x,
                             targetBlock->offset.y, targetBlock->offset.z + 1);
      lightLevel = GetBlockLightFromMap(t_terrain, targetBlock->offset.x,
                                        targetBlock->offset.y,
                                        targetBlock->offset.z + 1);
      break;

    case FACE_SIDE::FRONT:
      sunLightLevel =
          GetSunLightFromMap(t_terrain, targetBlock->offset.x,
                             targetBlock->offset.y, targetBlock->offset.z - 1);
      lightLevel = GetBlockLightFromMap(t_terrain, targetBlock->offset.x,
                                        targetBlock->offset.y,
                                        targetBlock->offset.z - 1);
      break;

    default:
      return;
  }

  /**
   *  I've built this formula:
   * (intensity + (lightLevel / MAX_LIGHT_VALUE)) / intensity + 1.0;
   */
  // const float sunLightFactor = (intensity + (sunLightLevel /
  // MAX_LIGHT_VALUE)) / (intensity + 1.0F);
  const float sunLightFactor =
      (sunLightLevel * sunlightIntensity) / MAX_LIGHT_VALUE;

  const float lightLevelFactor = lightLevel / MAX_LIGHT_VALUE;

  *baseColor = LightManager::IntensifyColor(
      baseColor, std::max(sunLightFactor, lightLevelFactor));

  // printf("Sunlight lvl: %d | intensity: %f\n", lightLevel, sunLightFactor);
  // printf("SunlightIntensity: %f\n", sunlightIntensity);
}
