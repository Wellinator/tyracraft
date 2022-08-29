#pragma once

#include <engine.hpp>
#include <game.hpp>
#include <debug/debug.hpp>
#include <tamtypes.h>
#include "renderer/3d/mesh/mesh.hpp"
#include "entities/chunck.hpp"
#include "entities/Block.hpp"
#include "camera.hpp"
#include "managers/terrain_manager.hpp"
#include "managers/state_manager.hpp"
#include "renderer/3d/pipeline/static/static_pipeline.hpp"

namespace TyraCraft {

class TyraCraftGame : public Tyra::Game {
 public:
  TyraCraftGame(Tyra::Engine* engine);
  ~TyraCraftGame();

  void init();
  void loop();

 private:
  Camera camera;
  StateManager* stateManager;
  Vec4 cameraPosition, cameraLookAt;

  Tyra::Engine* engine;
};

}  // namespace TyraCraft
