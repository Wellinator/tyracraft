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
  void update();
  void render();

 private:
  WorldLightModel* worldLightModel;

  Color baseColor = Color(128.0F, 128.0F, 128.0F, 96.0F);
  Color tempColor;

  const u8 DRAW_DATA_COUNT = 6;
  Vec4* vertices = new Vec4[DRAW_DATA_COUNT]{
      Vec4(-1.0F, 1.0F, -1.0), Vec4(1.0F, 1.0F, 1.0),  Vec4(1.0F, 1.0F, -1.0),
      Vec4(-1.0F, 1.0F, -1.0), Vec4(-1.0F, 1.0F, 1.0), Vec4(1.0F, 1.0F, 1.0),
  };

  std::vector<Vec4> uvMap;

  StaticPipeline stapip;
  Renderer* t_renderer = nullptr;
  Texture* cloudsTex = nullptr;

  float XMap = 1;
  float YMap = 1;

  void calcUVMapping();
  void updateCloudsPosition();
};
