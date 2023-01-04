#include "managers/day_night_cycle_manager.hpp"

DayNightCycleManager::DayNightCycleManager() {}

DayNightCycleManager::~DayNightCycleManager() {}

void DayNightCycleManager::update() {
  updateCurrentAngle();
  updateIntensityByAngle();
  updateEntitiesPosition();
}

const float DayNightCycleManager::getLightIntensity() {
  float intensity = getLightScaleFromAngle();

  intensity *= isDay() ? dayLightIntensity : nightLightIntensity;
  return baseLightIntensity + intensity;
}

const float DayNightCycleManager::getAmbientLightIntesity() {
  float intensity = getLightScaleFromAngle();
  intensity *= isDay() ? dayAmbientLightIntesity : nightAmbientLightIntesity;
  return baseAmbientLightIntensity + intensity;
}

void DayNightCycleManager::updateCurrentAngle() {
  currentAngleInDegrees = (g_ticksCounter / DAY_DURATION_IN_TICKS) * 360;
}

void DayNightCycleManager::updateEntitiesPosition() {
  const float angle = currentAngleInDegrees;
  Vec4 offset =
      center + Vec4(Math::sin(Math::ANG2RAD * angle) * circleRad,
                    Math::cos(Math::ANG2RAD * angle) * circleRad, 0.0F);

  sunPosition.set(offset);
  moonPosition.set(-sunPosition);
}

const Color DayNightCycleManager::getSkyColor() {
  Color result;
  float interpolation = getLightScaleFromAngle();

  isDay()
      ? result.lerp(AFTERNOON_MORNING_COLOR, DAY_MID_COLOR, interpolation)
      : result.lerp(AFTERNOON_MORNING_COLOR, NIGHT_MID_COLOR, interpolation);

  return result;
}

void DayNightCycleManager::updateIntensityByAngle() {
  _intensity = std::abs(Math::sin(currentAngleInDegrees * Math::ANG2RAD));
}
