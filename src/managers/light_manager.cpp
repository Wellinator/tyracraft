#include "managers/light_manager.hpp"

LightManager::LightManager(Level* level) { pLevel = level; }

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

Color LightManager::IntensifyColor(const Color& color, const float intensity) {
  return Color(color.r * intensity, color.g * intensity, color.b * intensity,
               color.a);
}

void LightManager::IntensifyColor(Color* color, const float intensity) {
  color->r *= intensity;
  color->g *= intensity;
  color->b *= intensity;

  color->r = std::min(color->r, 255.0f);
  color->g = std::min(color->g, 255.0f);
  color->b = std::min(color->b, 255.0f);
}

void LightManager::ApplyLightToFace(Color* baseColor, Vec4* offset,
                                    FACE_SIDE faceSide, Level* pLevel,
                                    const float sunlightIntensity) {
  const float MAX_LIGHT_VALUE = 15.0F;
  const float MIN_LIGHT_FACTOR = 0.15F;

  u8 lightData;
  u8 sunLightLevel;
  u8 lightLevel;

  switch (faceSide) {
    case FACE_SIDE::TOP:
      lightData =
          pLevel->GetLightDataFromMap(offset->x, offset->y + 1, offset->z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::BOTTOM:
      lightData =
          pLevel->GetLightDataFromMap(offset->x, offset->y - 1, offset->z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::LEFT:
      lightData =
          pLevel->GetLightDataFromMap(offset->x + 1, offset->y, offset->z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::RIGHT:
      lightData =
          pLevel->GetLightDataFromMap(offset->x - 1, offset->y, offset->z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::BACK:
      lightData =
          pLevel->GetLightDataFromMap(offset->x, offset->y, offset->z + 1);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::FRONT:
      lightData =
          pLevel->GetLightDataFromMap(offset->x, offset->y, offset->z - 1);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    default:
      return;
  }

  const float sunLightFactor = std::max(
      (sunLightLevel * sunlightIntensity) / MAX_LIGHT_VALUE, MIN_LIGHT_FACTOR);

  const float lightLevelFactor = lightLevel / MAX_LIGHT_VALUE;

  *baseColor = LightManager::IntensifyColor(
      *baseColor, std::max(sunLightFactor, lightLevelFactor));
}

void LightManager::ApplyLightToFace(Color* baseColor, Vec4* offset,
                                    Level* pLevel,
                                    const float sunlightIntensity) {
  const float MAX_LIGHT_VALUE = 15.0F;
  const float MIN_LIGHT_FACTOR = 0.15F;

  u8 lightData = pLevel->GetLightDataFromMap(offset->x, offset->y, offset->z);
  u8 sunLightLevel = ((lightData >> 4) & 0xF);
  u8 lightLevel = lightData & 0x0F;

  const float sunLightFactor = std::max(
      (sunLightLevel * sunlightIntensity) / MAX_LIGHT_VALUE, MIN_LIGHT_FACTOR);

  const float lightLevelFactor = lightLevel / MAX_LIGHT_VALUE;

  *baseColor = LightManager::IntensifyColor(
      *baseColor, std::max(sunLightFactor, lightLevelFactor));
}

void LightManager::ApplyLightToAllFaces(const Color& baseFaceColor,
                                        const Vec4* offset,
                                        u8 visibleFaces,
                                        const FACE_SIDE faceSides[6],
                                        const float faceIntensities[6],
                                        Level* pLevel,
                                        float sunlightIntensity,
                                        Color outColors[6]) {
  const float MAX_LIGHT_VALUE = 15.0F;
  const float MIN_LIGHT_FACTOR = 0.15F;

  // Neighbor offsets indexed by FACE_SIDE enum value.
  // FACE_SIDE: FRONT=0, BACK=1, LEFT=2, RIGHT=3, TOP=4, BOTTOM=5
  // Directions from original ApplyLightToFace switch/case:
  //   FRONT → z-1, BACK → z+1, LEFT → x+1, RIGHT → x-1, TOP → y+1, BOTTOM → y-1
  static const int dx[6] = {0, 0, 1, -1, 0, 0};
  static const int dy[6] = {0, 0, 0, 0, 1, -1};
  static const int dz[6] = {-1, 1, 0, 0, 0, 0};

  // BlockFace bit flags for each face index (0=TOP, 1=BOTTOM, 2=LEFT, 3=RIGHT,
  // 4=BACK, 5=FRONT)
  // These match the order used in CuboidMeshBuilder_loadLightData
  static const u8 faceBits[6] = {
      0b000010,  // TOP
      0b000001,  // BOTTOM
      0b001000,  // LEFT
      0b000100,  // RIGHT
      0b010000,  // BACK
      0b100000   // FRONT
  };

  const int bx = (int)offset->x;
  const int by = (int)offset->y;
  const int bz = (int)offset->z;

  for (int f = 0; f < 6; ++f) {
    if (!(visibleFaces & faceBits[f])) continue;

    // Apply face shading intensity
    Color faceColor = IntensifyColor(baseFaceColor, faceIntensities[f]);

    // Lookup light data from the neighbor in the face direction
    const int side = static_cast<int>(faceSides[f]);
    u8 lightData =
        pLevel->GetLightDataFromMap(bx + dx[side], by + dy[side], bz + dz[side]);
    u8 sunLightLevel = (lightData >> 4) & 0xF;
    u8 lightLevel = lightData & 0x0F;

    const float sunLightFactor = std::max(
        (sunLightLevel * sunlightIntensity) / MAX_LIGHT_VALUE, MIN_LIGHT_FACTOR);
    const float lightLevelFactor = lightLevel / MAX_LIGHT_VALUE;

    outColors[f] =
        IntensifyColor(faceColor, std::max(sunLightFactor, lightLevelFactor));
  }
}

Color LightManager::GetLightColorAt(const Vec4& offset, TargetedFace face,
                                    const float sunlightIntensity) {
  const float MAX_LIGHT_VALUE = 15.0F;
  const float MIN_LIGHT_FACTOR = 0.15F;

  Color baseFaceColor = Color(120, 120, 120, 128);
  Color finalColor = baseFaceColor;

  Vec4 faceOffset = Vec4(offset);

  if (face == TargetedFace::FrontFace) {
    finalColor = LightManager::IntensifyColor(finalColor, 0.8F);
    faceOffset.z++;
  } else if (face == TargetedFace::BackFace) {
    finalColor = LightManager::IntensifyColor(finalColor, 0.8F);
    faceOffset.z--;
  } else if (face == TargetedFace::RightFace) {
    finalColor = LightManager::IntensifyColor(finalColor, 0.6F);
    faceOffset.x++;
  } else if (face == TargetedFace::LeftFace) {
    finalColor = LightManager::IntensifyColor(finalColor, 0.6F);
    faceOffset.x--;
  } else if (face == TargetedFace::TopFace) {
    finalColor = LightManager::IntensifyColor(finalColor, 1.0F);
    faceOffset.y++;
  } else if (face == TargetedFace::BottomFace) {
    faceOffset.y--;
    finalColor = LightManager::IntensifyColor(finalColor, 0.5F);
  }

  Level* pLevel = Level::getInstance();
  u8 lightData =
      pLevel->GetLightDataFromMap(faceOffset.x, faceOffset.y, faceOffset.z);
  u8 sunLightLevel = ((lightData >> 4) & 0xF);
  u8 lightLevel = lightData & 0x0F;

  const float sunLightFactor = std::max(
      (sunLightLevel * sunlightIntensity) / MAX_LIGHT_VALUE, MIN_LIGHT_FACTOR);

  const float lightLevelFactor = lightLevel / MAX_LIGHT_VALUE;

  return LightManager::IntensifyColor(
      finalColor, std::max(sunLightFactor, lightLevelFactor));
}