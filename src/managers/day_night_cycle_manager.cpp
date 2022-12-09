#include "managers/day_night_cycle_manager.hpp"

DayNightCycleManager::DayNightCycleManager() {}

DayNightCycleManager::~DayNightCycleManager() {}

void DayNightCycleManager::update(const float& deltaTime) {
  updateTicks(std::min(deltaTime, MAX_FRAME_MS));
  updateCurrentAngle();
  updateEntitiesPosition();
}

const float DayNightCycleManager::getLightIntensity() {
  float intensity = getLightScaleFromAngle();
  intensity *= (ticksCounter < DAY_SUNSET || ticksCounter > DAY_SUNRISE)
                   ? dayLightIntensity
                   : nightLightIntensity;
  return baseLightIntensity + intensity;
}

const float DayNightCycleManager::getAmbientLightIntesity() {
  float intensity = getLightScaleFromAngle();
  intensity *= ambientLightIntesity;
  return baseLightIntensity + intensity;
}

void DayNightCycleManager::updateCurrentAngle() {
  currentAngleInDegrees = (ticksCounter / DAY_DURATION_IN_TICKS) * 360;
}

void DayNightCycleManager::updateEntitiesPosition() {
  const float angle = currentAngleInDegrees;
  Vec4 offset =
      center + Vec4(Math::sin(Math::ANG2RAD * angle) * circleRad,
                    Math::cos(Math::ANG2RAD * angle) * circleRad, 0.0F);

  sunPosition.set(offset);
  moonPosition.set(-sunPosition);
}

void DayNightCycleManager::updateTicks(const float& deltaTime) {
  elapsedRealTime += deltaTime;
  ticksCounter += deltaTime / tick;

  if (ticksCounter >= DAY_DURATION_IN_TICKS) {
    ticksDayCounter++;
    ticksCounter -= DAY_DURATION_IN_TICKS;
  }
}

const Color DayNightCycleManager::getSkyColor() {
  Color result;
  Color colorA = Color(120, 169, 255);
  Color colorB = Color(0, 0, 0);
  float interpolation = getLightScaleFromAngle();

  result.lerp(colorA, colorB, interpolation);
  return result;
}
