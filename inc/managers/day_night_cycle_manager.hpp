#pragma once
#include <tamtypes.h>
#include "utils.hpp"
#include "tyra"
#include <cmath>
#include "constants.hpp"
#include "managers/tick_manager.hpp"

#define DAY_MID_COLOR Color(120, 169, 255)
#define AFTERNOON_MORNING_COLOR Color(60, 85, 128)
#define NIGHT_MID_COLOR Color(20, 20, 50)

using Tyra::Color;
using Tyra::Math;
using Tyra::Vec4;

class DayNightCycleManager {
 public:
  DayNightCycleManager();
  ~DayNightCycleManager();
  void update();

  float currentAngleInDegrees = 0.0F;
  float angularSpeed = 5.0F;
  float circleRad = HALF_OVERWORLD_H_DISTANCE * DUBLE_BLOCK_SIZE;

  inline const Vec4 getSunPosition() { return sunPosition; };
  inline const Vec4 getMoonPosition() { return moonPosition; };

  /**
   * @brief base on https://minecraft.fandom.com/wiki/Daylight_cycle
   *
   */
  const Color getSkyColor();

  const float getLightIntensity();
  const float getAmbientLightIntesity();

 private:
  void updateEntitiesPosition();
  void updateCurrentAngle();
  inline const float getLightScaleFromAngle() {
    return std::abs((currentAngleInDegrees - 180.0F) / 180.0F);
  };

  Vec4 center = CENTER_WORLD_POS;
  Vec4 sunPosition;
  Vec4 moonPosition;

  float baseLightIntensity = 35.0F;
  float dayLightIntensity = 50.0F;
  float ambientLightIntesity = 20.0F;
  float nightLightIntensity = 20.0F;
};
