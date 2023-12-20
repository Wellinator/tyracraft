#pragma once
#include "tyra"

using Tyra::Color;
using Tyra::Vec4;

class WorldLightModel {
 public:
  Color ambientLight;
  Vec4 sunPosition;
  Vec4 moonPosition;
  float sunLightIntensity;
};
