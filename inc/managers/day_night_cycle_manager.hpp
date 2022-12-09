#pragma once
#include <tamtypes.h>
#include "utils.hpp"
#include "tyra"
#include <cmath>
#include "constants.hpp"

using Tyra::Color;
using Tyra::Math;
using Tyra::Vec4;

class DayNightCycleManager {
 public:
  DayNightCycleManager();
  ~DayNightCycleManager();
  void update(const float& deltaTime);

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

  // TODO: move to ticksManager
  double elapsedRealTime = 0.0F;
  const float tick = 1.0F / 20.0F;
  const float DAY_DURATION_IN_TICKS = 24000.0F;

  // Values in ticks
  const int DAY_INIT = 0;
  const int DAY_MID = 6000;
  const int DAY_SUNSET = 12000;
  const int NIGHT_INIT = 13000;
  const int NIGHT_MID = 18000;
  const int DAY_SUNRISE = 23000;
  const int DAY_END = 240000;

  float ticksCounter = DAY_MID;
  u16 ticksDayCounter = 0;
  void updateTicks(const float& deltaTime);

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
