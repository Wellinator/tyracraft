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
      return 0.35F;

    case 1:
      return 0.5F;

    case 2:
      return 0.6F;

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
  const float MIN_LIGHT_FACTOR = 0.15F;

  u8 lightData;
  u8 sunLightLevel;
  u8 lightLevel;

  Vec4 targetBlockOffset;
  GetXYZFromPos(&targetBlock->offset, &targetBlockOffset);

  switch (faceSide) {
    case FACE_SIDE::TOP:
      lightData =
          GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                              targetBlockOffset.y + 1, targetBlockOffset.z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::BOTTOM:
      lightData =
          GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                              targetBlockOffset.y - 1, targetBlockOffset.z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::LEFT:
      lightData = GetLightDataFromMap(t_terrain, targetBlockOffset.x + 1,
                                      targetBlockOffset.y, targetBlockOffset.z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::RIGHT:
      lightData = GetLightDataFromMap(t_terrain, targetBlockOffset.x - 1,
                                      targetBlockOffset.y, targetBlockOffset.z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::BACK:
      lightData =
          GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                              targetBlockOffset.y, targetBlockOffset.z + 1);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::FRONT:
      lightData =
          GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                              targetBlockOffset.y, targetBlockOffset.z - 1);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
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
  const float sunLightFactor = std::max(
      (sunLightLevel * sunlightIntensity) / MAX_LIGHT_VALUE, MIN_LIGHT_FACTOR);

  const float lightLevelFactor = lightLevel / MAX_LIGHT_VALUE;

  *baseColor = LightManager::IntensifyColor(
      baseColor, std::max(sunLightFactor, lightLevelFactor));

  // printf("Sunlight lvl: %d | intensity: %f\n", lightLevel, sunLightFactor);
  // printf("SunlightIntensity: %f\n", sunlightIntensity);
}

void LightManager::ApplyLightToFace(Color* baseColor, Block* targetBlock,
                                    LevelMap* t_terrain,
                                    const float sunlightIntensity) {
  const float MAX_LIGHT_VALUE = 15.0F;
  const float MIN_LIGHT_FACTOR = 0.15F;

  Vec4 targetBlockOffset;
  GetXYZFromPos(&targetBlock->offset, &targetBlockOffset);

  u8 lightData = GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                                     targetBlockOffset.y, targetBlockOffset.z);
  u8 sunLightLevel = ((lightData >> 4) & 0xF);
  u8 lightLevel = lightData & 0x0F;

  /**
   *  I've built this formula:
   * (intensity + (lightLevel / MAX_LIGHT_VALUE)) / intensity + 1.0;
   */
  // const float sunLightFactor = (intensity + (sunLightLevel /
  // MAX_LIGHT_VALUE)) / (intensity + 1.0F);
  const float sunLightFactor = std::max(
      (sunLightLevel * sunlightIntensity) / MAX_LIGHT_VALUE, MIN_LIGHT_FACTOR);

  const float lightLevelFactor = lightLevel / MAX_LIGHT_VALUE;

  *baseColor = LightManager::IntensifyColor(
      baseColor, std::max(sunLightFactor, lightLevelFactor));

  // printf("Sunlight lvl: %d | intensity: %f\n", lightLevel, sunLightFactor);
  // printf("SunlightIntensity: %f\n", sunlightIntensity);
}
