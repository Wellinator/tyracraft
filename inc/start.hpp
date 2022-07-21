
#include <debug/debug.hpp>
#include <tamtypes.h>
#include <game.hpp>
#include <engine.hpp>
#include "renderer/3d/mesh/mesh.hpp"
#include "entities/chunck.hpp"
#include "entities/Block.hpp"
#include "camera.hpp"
#include "managers/terrain_manager.hpp"
#include "managers/state_manager.hpp"

#pragma once

using Tyra::Audio;
using Tyra::Game;
using Tyra::Engine;

class Start : public Game {
 private:
  Camera camera;
  StateManager stateManager;

 public:
  Engine* engine;

  Start(Engine* t_engine);
  ~Start();

  void init();
  void loop();

};
