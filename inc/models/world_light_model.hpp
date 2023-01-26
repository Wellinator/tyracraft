#pragma once
#include "tyra"

using Tyra::Color;
using Tyra::Vec4;

class WorldLightModel {
 public:
  Vec4* lightsPositions;
  Color ambientLight;
  Vec4 sunPosition;
  Vec4 moonPosition;
  float lightIntensity;
  float ambientLightIntensity;
};
