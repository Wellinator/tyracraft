// Based on https://gist.github.com/luuthevinh/42227ad9712e86ab9d5c3ab37a56936c

#pragma once

#include "constants.hpp"
#include <tyra>
#include <math.h>

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

  void init(Renderer* renderer);
  void update();
  void render();

 private:
  std::vector<Vec4> vertices = {
      Vec4(-1, -1, 0), Vec4(-1, 1, 0), Vec4(1, 1, 0),
      Vec4(-1, -1, 0), Vec4(1, 1, 0),  Vec4(1, -1, 0),
  };

  std::vector<Vec4> verticesNormals = {
      Vec4(0, -1, 0), Vec4(0, -1, 0), Vec4(0, -1, 0),
      Vec4(0, -1, 0), Vec4(0, -1, 0), Vec4(0, -1, 0),
  };

  std::vector<Vec4> uvMap;

  StaticPipeline stapip;
  Renderer* t_renderer = nullptr;
  Texture* cloudsTex = nullptr;
};
