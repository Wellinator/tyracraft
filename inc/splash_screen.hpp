/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include <renderer/renderer.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <renderer/core/2d/sprite/sprite.hpp>
#include <renderer/core/3d/camera_info_3d.hpp>
#include <tamtypes.h>
#include <sifrpc.h>
#include <debug.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include "contants.hpp"

#pragma once

using Tyra::Renderer;
using Tyra::Sprite;

class SplashScreen {
 public:
  SplashScreen(Renderer* renderer);
  ~SplashScreen();

  void render();
  u8 shouldBeDestroyed();

 private:
  void setBgColorBlack(Renderer* renderer);
  void renderTyraSplash();
  void renderTyraCraftSplash();
  u8 hasFinished();

  Renderer* t_renderer;
  Sprite** tyracraft_grid = new Sprite*[16];
  Sprite** tyra_grid = new Sprite*[16];
  Sprite* sprite = new Sprite;
  u8 alpha = 1;
  u8 isFading = 0;
  u8 hasShowedTyraCraft = 0;
  u8 hasShowedTyra = 0;
};
