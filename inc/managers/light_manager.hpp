#pragma once

#include "constants.hpp"
#include <tyra>
#include <math.h>
#include <array>
#include "entities/level.hpp"
#include "entities/Block.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Vec4;

class LightManager {
 public:
  LightManager(Level* pLevel);
  ~LightManager();

  void init();
  void update();

  static std::array<u8, 4> getCornersAOValues(
      std::array<u8, 8> blocksNeightbors);
  static u8 getVertexAO(bool side1, bool corner, bool side2);
  static float calcAOIntensity(u8 AOValue);
  static Color GetLightColorAt(const Vec4& offset, TargetedFace face,
                               const float sunlightIntensity);
  static Color IntensifyColor(const Color& color, const float intensity);
  static void IntensifyColor(Color* color, const float intensity);
  static void ApplyLightToFace(Color* baseColor, Vec4* offset,
                               FACE_SIDE faceSide, Level* pLevel,
                               const float sunlightIntensity);
  static void ApplyLightToFace(Color* baseColor, Vec4* offset, Level* pLevel,
                               const float sunlightIntensity);

  // Batch version: computes light for all visible faces in one call.
  // faceSides[6] maps each BlockFace index to its world FACE_SIDE.
  // TOP/BOTTOM are always direct; LEFT/RIGHT/BACK/FRONT use rotation mapping.
  // outColors[6] receives the lit Color for each visible face.
  static void ApplyLightToAllFaces(const Color& baseFaceColor,
                                   const Vec4* offset,
                                   u8 visibleFaces,
                                   const FACE_SIDE faceSides[6],
                                   const float faceIntensities[6],
                                   Level* pLevel,
                                   float sunlightIntensity,
                                   Color outColors[6]);

 private:
  Level* pLevel = nullptr;
};
