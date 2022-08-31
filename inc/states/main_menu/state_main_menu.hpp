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
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/screens/screen_main.hpp"
#include "models/new_game_model.hpp"

using Tyra::Audio;
using Tyra::Mesh;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::StaPipOptions;
using Tyra::StaticMesh;
using Tyra::StaticPipeline;

class ScreenBase;
class StateMainMenu : public GameState {
 public:
  StateMainMenu(Context* context);
  ~StateMainMenu();

  void init();
  void update(const float& deltaTime);
  void render();
  void setScreen(ScreenBase* screen);

  // Rotating skybox
  StaticMesh* menuSkybox;

  void loadGame(const NewGameOptions& options);

 private:
  Audio* t_audio;

  Renderer* t_renderer;
  Sprite title[2];

  StaPipOptions* skyboxOptions;
  StaticPipeline stapip;

  ScreenBase* screen = nullptr;

  const float SLOT_WIDTH = 160;

  u8 hasFinished();
  u8 shouldInitGame();
  void loadSkybox(Renderer* renderer);
  void unloadTextures();
  void loadMenuSong();

  // Scrrens
  void loadMainScreen();
};
