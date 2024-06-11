#pragma once
#include <tamtypes.h>
#include "utils.hpp"
#include "tyra"
#include <cmath>
#include "constants.hpp"
#include "managers/tick_manager.hpp"

#define DAY_MID_COLOR Color(120.0f, 169.0f, 255.0f)
#define AFTERNOON_MORNING_COLOR Color(45.0f, 64.0f, 97.0f)
#define NIGHT_MID_COLOR Color(10.0f, 10.0f, 25.0f)

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
  void update(const float deltaTime, const Vec4* camPos);
  void tick();
  void render();

  float currentAngleInDegrees = 0.0F;

  inline const Vec4 getSunPosition() { return sunPosition; };
  inline const Vec4 getMoonPosition() { return moonPosition; };

  /**
   * @brief base on https://minecraft.fandom.com/wiki/Daylight_cycle
   *
   */
  const Color getSkyColor();
  void setSkyColor(Color midDaycolor, Color afterNoonAndMorningColor,
                   Color midNight);
  void resetSkyColor();

  const float getSunLightIntensity();

 private:
  Renderer* t_renderer;
  Texture* sunTexture;
  Texture* moonTexture;

  StaticPipeline stapip;

  void loadTextures();
  void loadDrawData();
  void updateSunDrawData(const Vec4& camPos);
  void updateMoonDrawData(const Vec4& camPos);
  void renderSun();
  void renderMoon();

  void calNextEntitiesPosition();
  void updateEntitiesPosition();
  void updateCurrentAngle();
  void updateIntensityByAngle();
  inline const float getLightScaleFromAngle() { return _intensity; };

  const float distance = HALF_OVERWORLD_H_DISTANCE * DUBLE_BLOCK_SIZE;
  Vec4 center = CENTER_WORLD_POS;

  float lerp = 0.0f;

  Vec4 sunPosition = Vec4(0, 0, 0);
  Vec4 sunPositionStart = Vec4(0, 0, 0);
  Vec4 sunPositionEnd = Vec4(0, 0, 0);

  Vec4 moonPosition = Vec4(0, 0, 0);
  Vec4 moonPositionStart = Vec4(0, 0, 0);
  Vec4 moonPositionEnd = Vec4(0, 0, 0);

  float _intensity;

  Color _midDaycolor = DAY_MID_COLOR;
  Color _afterNoonAndMorningColor = AFTERNOON_MORNING_COLOR;
  Color _midNight = NIGHT_MID_COLOR;

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
