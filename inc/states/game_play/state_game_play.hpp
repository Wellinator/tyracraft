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

using Tyra::Audio;
using Tyra::Mesh;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::StaPipOptions;
using Tyra::StaticMesh;
using Tyra::StaticPipeline;

enum class GameMode { Survival, Creative };

class StateGamePlay : public GameState {
 public:
  StateGamePlay(Context* context);
  ~StateGamePlay();

  void init();
  void update(const float& deltaTime);
  void render();

  // Rotating skybox
  StaticMesh* menuSkybox;

 private:
  std::chrono::steady_clock::time_point lastTimeCrossWasClicked;
  GameMode gameMode = GameMode::Survival;

  void unload();
  void handleInput();
  void controlGameMode();
};
