
#pragma once

#include <debug/debug.hpp>
#include <renderer/renderer.hpp>
#include <renderer/core/2d/sprite/sprite.hpp>
#include <math/vec4.hpp>
#include <math.h>
#include "contants.hpp"

using Tyra::Color;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Vec4;

enum class LoadingState { Loading = 0, Complete = 1 };

class LoadingScreen {
 public:
  LoadingScreen();
  ~LoadingScreen();

  void init(Renderer* t_renderer);
  void setState(LoadingState state);
  void setPercent(float completed);
  void render();
  void unload();
  u8 hasFinished();
  u8 shouldBeDestroyed();

 private:
  Renderer* t_renderer;
  Sprite* background;
  Sprite* loadingSlot;
  Sprite* loadingprogress;
  Sprite* loadingStateText;
  LoadingState _state = LoadingState::Loading;
  float _percent = 1.0F;
  float BASE_HEIGHT;

  void setBgColorBlack(Renderer* renderer);
};
