#pragma once
#include <tamtypes.h>
#include "utils.hpp"
#include "tyra"
#include <cmath>
#include "constants.hpp"
#include "managers/tick_manager.hpp"

#define DAY_MID_COLOR Color(120, 169, 255)
#define AFTERNOON_MORNING_COLOR Color(45, 64, 97)
#define NIGHT_MID_COLOR Color(10, 10, 25)

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::Math;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::Texture;
using Tyra::Vec4;

class DayNightCycleManager {
 public:
  DayNightCycleManager();
  ~DayNightCycleManager();
  void init(Renderer* renderer);
  void preLoad();
  void update();
  void tick(const Vec4* camPos);
  void render();

  float currentAngleInDegrees = 0.0F;

  inline const Vec4 getSunPosition() { return sunPosition; };
  inline const Vec4 getMoonPosition() { return moonPosition; };

  /**
   * @brief base on https://minecraft.fandom.com/wiki/Daylight_cycle
   *
   */
  const Color getSkyColor();

  const float getSunLightIntensity();

 private:
  Renderer* t_renderer;
  Texture* sunTexture;
  Texture* moonTexture;

  StaticPipeline stapip;

  void loadTextures();
  void loadDrawData();
  void updateSunDrawData(const Vec4* camPos);
  void updateMoonDrawData(const Vec4* camPos);
  void renderSun();
  void renderMoon();

  void updateEntitiesPosition(const Vec4* camPos);
  void updateCurrentAngle();
  void updateIntensityByAngle();
  inline const float getLightScaleFromAngle() { return _intensity; };

  const float distance = HALF_OVERWORLD_H_DISTANCE * DUBLE_BLOCK_SIZE;
  Vec4 center = CENTER_WORLD_POS;
  Vec4 sunPosition;
  Vec4 moonPosition;

  float _intensity;

  const u8 DRAW_DATA_COUNT = 6;

  const float baseLightIntensity = 25.0F;
  const float dayLightIntensity = 45.0F;
  const float nightLightIntensity = 15.0F;

  const float baseAmbientLightIntensity = 25.0F;
  const float dayAmbientLightIntesity = 30.0F;
  const float nightAmbientLightIntesity = 10.0F;

  M4x4 sunScale;
  M4x4 moonScale;
  Color baseColor = Color(128, 128, 128);

  std::array<Vec4, 6> sunUVMap;
  std::array<Vec4, 6> sunVertexData;

  std::array<Vec4, 6> moonUVMap;
  std::array<Vec4, 6> moonVertexData;

  const float xMin = 0.0F;
  const float xMax = 1.0F;
  const float yMin = 0.0F;
  const float yMax = 1.0F;
};
