#pragma once
#include <file/file_utils.hpp>
#include <string.h>
#include "renderer/models/color.hpp"
#include <debug/debug.hpp>
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
#include "constants.hpp"
#include "states/game_state.hpp"
#include "states/context.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Sprite;
using Tyra::TextureRepository;

class StateSplashScreen : public GameState {
 public:
  StateSplashScreen(Context* context);
  ~StateSplashScreen();

  void init();
  void update(const float& deltaTime);
  void render();

 private:
  void unloadTextures();
  void setBgColorBlack(Renderer* renderer);
  void renderTyraSplash();
  void renderTyraCraftSplash();
  u8 hasFinished();
  void nextState();

  Sprite* tyracraft = new Sprite;
  Sprite* tyra = new Sprite;
  u8 alpha = 1;
  u8 isFading = 0;
  u8 hasShowedTyraCraft = 0;
  u8 hasShowedTyra = 0;
};
