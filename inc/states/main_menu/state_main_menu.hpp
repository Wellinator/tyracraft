#pragma once

#include <debug/debug.hpp>
#include <renderer/renderer.hpp>
#include <pad/pad.hpp>
#include <audio/audio.hpp>
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
#include "camera.hpp"
#include "renderer/3d/mesh/static/static_mesh.hpp"
#include "renderer/3d/pipeline/static/static_pipeline.hpp"
#include "renderer/3d/pipeline/dynamic/dynamic_pipeline.hpp"
#include "states/game_state.hpp"
#include "states/context.hpp"

// MENU_OPTIONS
#define PLAY_GAME 1
#define ABOUT 2

using Tyra::Audio;
using Tyra::Mesh;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::StaPipOptions;
using Tyra::StaticMesh;
using Tyra::StaticPipeline;

class StateMainMenu : public GameState {
 public:
  StateMainMenu(Context* context);
  ~StateMainMenu();

  void init();
  void update();
  void render();
  u8 shouldInitGame();

  // Rotating skybox
  StaticMesh* menuSkybox;

 private:
  Audio* t_audio;
  u8 activeOption = PLAY_GAME;
  u8 selectedOption = 0;
  Renderer* t_renderer;
  Sprite title[2];
  Sprite slot[3];
  Sprite textPlayGame;
  Sprite textSelect;
  Sprite btnCross;
  StaPipOptions* skyboxOptions;
  StaticPipeline stapip;

  const float SLOT_WIDTH = 160;

  u8 hasFinished();
  void loadSkybox(Renderer* renderer);
  void unloadTextures();
  void handleInput();
};
