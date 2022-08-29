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

enum class MainMenuOptions { None, PlayGame, About };

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
  void update(const float& deltaTime);
  void render();

  // Rotating skybox
  StaticMesh* menuSkybox;

 private:
  Audio* t_audio;
  MainMenuOptions activeOption = MainMenuOptions::PlayGame;
  MainMenuOptions selectedOption = MainMenuOptions::None;
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
  u8 shouldInitGame();
  void loadSkybox(Renderer* renderer);
  void unloadTextures();
  void handleInput();
  void loadGame();
  void loadMenuSong();
};
