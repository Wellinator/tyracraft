#pragma once
#include <tamtypes.h>
#include "utils.hpp"
#include "tyra"
#include <cmath>
#include "constants.hpp"
#include "managers/tick_manager.hpp"

#define DAY_MID_COLOR Color(120, 169, 255)
#define AFTERNOON_MORNING_COLOR Color(60, 85, 128)
#define NIGHT_MID_COLOR Color(10, 10, 25)

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

  const float getSunLightIntensity();
  const float getAmbientLightIntesity();
  inline const u8 isDay() {
    return (g_ticksCounter > DAY_SUNRISE || g_ticksCounter < DAY_SUNSET);
  };

 private:
  void updateEntitiesPosition();
  void updateCurrentAngle();
  void updateIntensityByAngle();
  inline const float getLightScaleFromAngle() {
    return _intensity;
  };

  Vec4 center = CENTER_WORLD_POS;
  Vec4 sunPosition;
  Vec4 moonPosition;

  float _intensity;

  const float baseLightIntensity = 25.0F;
  const float dayLightIntensity = 45.0F;
  const float nightLightIntensity = 15.0F;

  const float baseAmbientLightIntensity = 25.0F;
  const float dayAmbientLightIntesity = 30.0F;
  const float nightAmbientLightIntesity = 10.0F;
};
