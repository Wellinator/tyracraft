#include "managers/day_night_cycle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "debug.hpp"
#include <array>

using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;

DayNightCycleManager::DayNightCycleManager() {}

DayNightCycleManager::~DayNightCycleManager() {
  t_renderer->getTextureRepository().free(sunTexture->id);
  t_renderer->getTextureRepository().free(moonTexture->id);
}

void DayNightCycleManager::init(Renderer* renderer) {
  updateCurrentAngle();
  updateIntensityByAngle();
  calNextEntitiesPosition();

  t_renderer = renderer;
  stapip.setRenderer(&renderer->core);
  loadTextures();
  loadDrawData();
}

void DayNightCycleManager::loadTextures() {
  sunTexture = t_renderer->core.texture.repository.add(
      FileUtils::fromCwd("/textures/environment/sun.png"));
  moonTexture = t_renderer->core.texture.repository.add(
      FileUtils::fromCwd("/textures/environment/moon.png"));

  // Assign textures to bags after they are loaded
  _sunBagTex.texture  = sunTexture;
  _moonBagTex.texture = moonTexture;
  _bagsReady = true;
}

void DayNightCycleManager::loadDrawData() {
  sunScale.identity();
  sunScale.scale(260.0F);

  moonScale.identity();
  moonScale.scale(230.0F);

  sunUVMap[0] = (Vec4(xMin, yMax, 1.0F, 0.0F));
  sunUVMap[1] = (Vec4(xMax, yMin, 1.0F, 0.0F));
  sunUVMap[2] = (Vec4(xMax, yMax, 1.0F, 0.0F));
  sunUVMap[3] = (Vec4(xMin, yMax, 1.0F, 0.0F));
  sunUVMap[4] = (Vec4(xMin, yMin, 1.0F, 0.0F));
  sunUVMap[5] = (Vec4(xMax, yMin, 1.0F, 0.0F));

  moonUVMap[0] = (Vec4(xMin, yMax, 1.0F, 0.0F));
  moonUVMap[1] = (Vec4(xMax, yMin, 1.0F, 0.0F));
  moonUVMap[2] = (Vec4(xMax, yMax, 1.0F, 0.0F));
  moonUVMap[3] = (Vec4(xMin, yMax, 1.0F, 0.0F));
  moonUVMap[4] = (Vec4(xMin, yMin, 1.0F, 0.0F));
  moonUVMap[5] = (Vec4(xMax, yMin, 1.0F, 0.0F));

  // --- Build persistent sun bag ---
  _sunBagMatrix.identity();
  _sunBagTex.coordinates = sunUVMap.data();   // pointer into member array
  // texture assigned once textures are loaded (see buildBagsAfterTextures)
  _sunBagInfo.model = &_sunBagMatrix;
  _sunBagInfo.blendingEnabled = true;
  _sunBagInfo.zTestType = Tyra::PipelineZTest::PipelineZTest_AllPass;
  _sunBagInfo.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  _sunBagColor.single = &baseColor;
  _sunBag.count = DRAW_DATA_COUNT;
  _sunBag.vertices = sunVertexData.data();    // pointer into member array
  _sunBag.color = &_sunBagColor;
  _sunBag.info = &_sunBagInfo;
  _sunBag.texture = &_sunBagTex;

  // --- Build persistent moon bag ---
  _moonBagMatrix.identity();
  _moonBagTex.coordinates = moonUVMap.data();
  _moonBagInfo.model = &_moonBagMatrix;
  _moonBagInfo.blendingEnabled = true;
  _moonBagInfo.zTestType = Tyra::PipelineZTest::PipelineZTest_AllPass;
  _moonBagInfo.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  _moonBagColor.single = &baseColor;
  _moonBag.count = DRAW_DATA_COUNT;
  _moonBag.vertices = moonVertexData.data();  // pointer into member array
  _moonBag.color = &_moonBagColor;
  _moonBag.info = &_moonBagInfo;
  _moonBag.texture = &_moonBagTex;
}

void DayNightCycleManager::updateSunDrawData(const Vec4& camPos) {
  std::array<Vec4, 6> rawData = {
      Vec4(1.0F, -1.0F, -1.0),
      Vec4(-1.0F, 1.0F, -1.0),
      Vec4(-1.0F, -1.0F, -1.0),
      Vec4(1.0F, 1.0F, -1.0),
  };

  M4x4 result, temp, model;
  M4x4::lookAt(&temp, sunPosition + camPos, camPos);
  Utils::inverseMatrix(&result, &temp);

  model.identity();
  model = result * sunScale;

  sunVertexData[0] = (model * rawData[0]);
  sunVertexData[1] = (model * rawData[1]);
  sunVertexData[2] = (model * rawData[2]);
  sunVertexData[3] = (model * rawData[0]);
  sunVertexData[4] = (model * rawData[3]);
  sunVertexData[5] = (model * rawData[1]);
}

void DayNightCycleManager::updateMoonDrawData(const Vec4& camPos) {
  std::array<Vec4, 6> rawData = {
      Vec4(1.0F, -1.0F, -1.0),
      Vec4(-1.0F, 1.0F, -1.0),
      Vec4(-1.0F, -1.0F, -1.0),
      Vec4(1.0F, 1.0F, -1.0),
  };

  M4x4 result, temp, model;
  M4x4::lookAt(&temp, moonPosition + camPos, camPos);
  Utils::inverseMatrix(&result, &temp);

  model.identity();
  model = result * moonScale;

  moonVertexData[0] = (model * rawData[0]);
  moonVertexData[1] = (model * rawData[1]);
  moonVertexData[2] = (model * rawData[2]);
  moonVertexData[3] = (model * rawData[0]);
  moonVertexData[4] = (model * rawData[3]);
  moonVertexData[5] = (model * rawData[1]);
}

void DayNightCycleManager::preLoad() {
  updateCurrentAngle();
  updateIntensityByAngle();
}

void DayNightCycleManager::update(const float deltaTime, const Vec4* camPos) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableDayNightCycle == false) return;
#endif  // DEBUG_MODE

  // The lerp must be between 0.0 and 1.0
  // it should be min (0) at the start of the tick and max (1) at the end of the
  // DAY_NIGHT_TICKS_UPDATE

  const float fiftyTicksInSeconds = DAY_NIGHT_TICKS_UPDATE * TICKS_IN_SECONDS;
  lerpAcc += deltaTime;
  lerp = lerpAcc / fiftyTicksInSeconds;

  if (lerp > 1.0f) lerp = 1.0f;
  if (lerp < 0.0f) lerp = 0.0f;

  // printf("Lerp: %f, lerpAcc: %f, deltaTime: %f\n", lerp, lerpAcc, deltaTime);

  updateCurrentAngle();
  updateIntensityByAngle();
  updateEntitiesPosition();

  if (g_ticksCounter > 22300 || g_ticksCounter < 13702) {
    updateSunDrawData(*camPos);
  }

  if (g_ticksCounter >= 11834 || g_ticksCounter < 167) {
    updateMoonDrawData(*camPos);
  }
}

void DayNightCycleManager::tick() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableDayNightCycle == false) return;
#endif  // DEBUG_MODE

  calNextEntitiesPosition();
}

/**
 * Based in https://minecraft.fandom.com/wiki/Daylight_cycle
 * Sun and Moon appears and disappears in the horizon
 */
void DayNightCycleManager::render() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableDayNightCycle == false) return;
#endif  // DEBUG_MODE

  if (!_bagsReady) return;

  t_renderer->renderer3D.usePipeline(stapip);

  // sunVertexData and moonVertexData are updated by update() each frame.
  // The persistent bags already hold direct pointers into those member arrays,
  // so no re-assignment is needed — just submit.
  if (g_ticksCounter > 22300 || g_ticksCounter < 13702) {
    stapip.core.render(&_sunBag);
  }

  if (g_ticksCounter >= 11834 || g_ticksCounter < 167) {
    stapip.core.render(&_moonBag);
  }
}

const float DayNightCycleManager::getSunLightIntensity() {
  return getLightScaleFromAngle();
}

void DayNightCycleManager::updateCurrentAngle() {
  currentAngleInDegrees =
      fmodf((g_ticksCounter / DAY_DURATION_IN_TICKS) * 360.0f, 360.0f);

  // Normalize to 0-360 range
  if (currentAngleInDegrees < 0) {
    currentAngleInDegrees += 360.0f;
  }
}

void DayNightCycleManager::calNextEntitiesPosition() {
  lerpAcc = 0.0;

  // Calculate current and next tick positions
  uint32_t currentDayTick = g_ticksCounter;
  uint32_t nextDayTick = g_ticksCounter + DAY_NIGHT_TICKS_UPDATE;

  // Calculate angles for current and next positions
  float angleStart =
      fmodf((currentDayTick / DAY_DURATION_IN_TICKS) * 360.0f, 360.0f);
  float angleEnd = fmodf((nextDayTick / DAY_DURATION_IN_TICKS) * 360.0f, 360.0f);

  // Normalize angles
  if (angleStart < 0) angleStart += 360.0f;
  if (angleEnd < 0) angleEnd += 360.0f;

  // Convert to radians
  float angleStartRad = Tyra::Math::ANG2RAD * angleStart;
  float angleEndRad = Tyra::Math::ANG2RAD * angleEnd;

  // Calculate sun positions (moves in a circular arc)
  // Y component: sin gives the height (DAY_MID)
  // Z component: cos gives the forward/back position
  sunPositionStart.set(0.0f, Math::sin(angleStartRad) * distance,
                       Math::cos(angleStartRad) * distance);
  sunPositionEnd.set(0.0f, Math::sin(angleEndRad) * distance,
                     Math::cos(angleEndRad) * distance);

  // Moon is 180 degrees opposite to the sun
  float moonAngleStartRad = angleStartRad + Tyra::Math::PI;
  float moonAngleEndRad = angleEndRad + Tyra::Math::PI;

  moonPositionStart.set(0.0f, Math::sin(moonAngleStartRad) * distance,
                        Math::cos(moonAngleStartRad) * distance);
  moonPositionEnd.set(0.0f, Math::sin(moonAngleEndRad) * distance,
                      Math::cos(moonAngleEndRad) * distance);
}

void DayNightCycleManager::updateEntitiesPosition() {
  Vec4::setLerp(&sunPosition, sunPositionStart, sunPositionEnd, lerp);
  Vec4::setLerp(&moonPosition, moonPositionStart, moonPositionEnd, lerp);
}

const Color DayNightCycleManager::getSkyColor() {
  Color result;
  const auto isDay = g_ticksCounter > DAY_INIT && g_ticksCounter < NIGHT_INIT;
  float interpolation = _intensity;

  isDay ? result.lerp(_afterNoonAndMorningColor, _midDaycolor, interpolation)
        : result.lerp(_midNight, _afterNoonAndMorningColor, interpolation);

  return result;
}

void DayNightCycleManager::setSkyColor(Color midDaycolor,
                                       Color afterNoonAndMorningColor,
                                       Color midNight) {
  _midDaycolor.set(midDaycolor);
  _afterNoonAndMorningColor.set(afterNoonAndMorningColor);
  _midNight.set(midNight);
}

void DayNightCycleManager::resetSkyColor() {
  _midDaycolor.set(DAY_MID_COLOR);
  _afterNoonAndMorningColor.set(AFTERNOON_MORNING_COLOR);
  _midNight.set(NIGHT_MID_COLOR);
}

void DayNightCycleManager::updateIntensityByAngle() {
  _intensity =
      Utils::reRangeScale(0.0F, 1.0F, -1.0F, 1.0F,
                          Math::sin(Math::ANG2RAD * currentAngleInDegrees));
}