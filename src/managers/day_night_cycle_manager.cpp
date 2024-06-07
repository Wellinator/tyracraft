#include "managers/day_night_cycle_manager.hpp"
#include "managers/tick_manager.hpp"
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
  lerp += deltaTime / 0.05;

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

void DayNightCycleManager::tick() { calNextEntitiesPosition(); }

/**
 * Based in https://minecraft.fandom.com/wiki/Daylight_cycle
 * Sun and Moon appears and disappears in the horizon
 */
void DayNightCycleManager::render() {
  t_renderer->renderer3D.usePipeline(stapip);

  if (g_ticksCounter > 22300 || g_ticksCounter < 13702) {
    renderSun();
  }

  if (g_ticksCounter >= 11834 || g_ticksCounter < 167) {
    renderMoon();
  }
}

void DayNightCycleManager::renderSun() {
  M4x4 rawMatrix;
  rawMatrix.identity();

  StaPipTextureBag textureBag;
  textureBag.texture = sunTexture;
  textureBag.coordinates = sunUVMap.data();

  StaPipInfoBag infoBag;
  infoBag.model = &rawMatrix;
  infoBag.blendingEnabled = true;
  infoBag.zTestType = Tyra::PipelineZTest::PipelineZTest_AllPass;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;

  StaPipColorBag colorBag;
  colorBag.single = &baseColor;

  StaPipBag bag;
  bag.count = sunVertexData.size();
  bag.vertices = sunVertexData.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  stapip.core.render(&bag);
}

void DayNightCycleManager::renderMoon() {
  M4x4 rawMatrix;
  rawMatrix.identity();

  StaPipTextureBag textureBag;
  textureBag.texture = moonTexture;
  textureBag.coordinates = moonUVMap.data();

  StaPipInfoBag infoBag;
  infoBag.model = &rawMatrix;
  infoBag.blendingEnabled = true;
  infoBag.zTestType = Tyra::PipelineZTest::PipelineZTest_AllPass;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;

  StaPipColorBag colorBag;
  colorBag.single = &baseColor;

  StaPipBag bag;
  bag.count = moonVertexData.size();
  bag.vertices = moonVertexData.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  stapip.core.render(&bag);
}

const float DayNightCycleManager::getSunLightIntensity() {
  return getLightScaleFromAngle();
}

void DayNightCycleManager::updateCurrentAngle() {
  currentAngleInDegrees = (g_ticksCounter / DAY_DURATION_IN_TICKS) * 360;
}

void DayNightCycleManager::calNextEntitiesPosition() {
  lerp = 0.0;

  const float angleStart = Tyra::Math::ANG2RAD * currentAngleInDegrees;
  Vec4 startDirection =
      Vec4(0.0F, Math::sin(angleStart), SIN_90 * Math::cos(angleStart))
          .getNormalized();

  const float angleEnd = Tyra::Math::ANG2RAD *
                         (((g_ticksCounter + 1) / DAY_DURATION_IN_TICKS) * 360);
  Vec4 endDirection =
      Vec4(0.0F, Math::sin(angleEnd), SIN_90 * Math::cos(angleEnd))
          .getNormalized();

  // Set next sun height position;
  sunPositionStart.set(startDirection * distance);
  sunPositionEnd.set(endDirection * distance);

  // Set next sun position;
  moonPositionStart.set(-startDirection * distance);
  moonPositionEnd = (-endDirection * distance);
}

void DayNightCycleManager::updateEntitiesPosition() {
  Vec4::setLerp(&sunPosition, sunPositionStart, sunPositionEnd, lerp);
  Vec4::setLerp(&moonPosition, moonPositionStart, moonPositionEnd, lerp);
}

const Color DayNightCycleManager::getSkyColor() {
  Color result;
  const auto isDay = g_ticksCounter > DAY_INIT && g_ticksCounter < NIGHT_INIT;
  float interpolation = _intensity;

  isDay ? result.lerp(AFTERNOON_MORNING_COLOR, DAY_MID_COLOR, interpolation)
        : result.lerp(NIGHT_MID_COLOR, AFTERNOON_MORNING_COLOR, interpolation);

  return result;
}

void DayNightCycleManager::updateIntensityByAngle() {
  _intensity =
      Utils::reRangeScale(0.0F, 1.0F, -1.0F, 1.0F,
                          Math::sin(Math::ANG2RAD * currentAngleInDegrees));
}
