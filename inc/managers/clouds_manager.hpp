#pragma once

#include "constants.hpp"
#include <tyra>
#include <math.h>
#include "models/world_light_model.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::Renderer;
using Tyra::Renderer3D;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;
using Tyra::Texture;
using Tyra::Vec4;

class CloudsManager {
 public:
  CloudsManager();
  ~CloudsManager();

  void init(Renderer* renderer, WorldLightModel* t_worldLightModel);
  void update(const float deltaTime);
  void tick();
  void render();

 private:
  WorldLightModel* worldLightModel;

  Color baseColor = Color(128.0F, 128.0F, 128.0F, 96.0F);
  Color tempColor;

  const u8 DRAW_DATA_COUNT = 6;
  Vec4 vertices[6] = {
      Vec4(-1.0F, 1.0F, -1.0), Vec4(1.0F, 1.0F, 1.0),  Vec4(1.0F, 1.0F, -1.0),
      Vec4(-1.0F, 1.0F, -1.0), Vec4(-1.0F, 1.0F, 1.0), Vec4(1.0F, 1.0F, 1.0),
  };

  Vec4 uvMap[6];

  StaticPipeline stapip;
  Renderer* t_renderer = nullptr;
  Texture* cloudsTex = nullptr;

  void calcUVMapping();
  void updateCloudsPosition();

  double lerp = 0.0;
  Vec4 position = Vec4(1.0f, 1.0f, 0);
  Vec4 positionStart = Vec4(0, 0, 0);
  Vec4 positionEnd = Vec4(0, 0, 0);
  Vec4 velocity = Vec4(0.001f, 0.0005f, 0);
  // Vec4 velocity = Vec4(0.001f, 0.0001f);
  const Vec4 scaleVec = Vec4(1.0F / 4.0F, 1.0F / 4.0F, 1.0F, 0.0F);
};
