#pragma once
#include <tamtypes.h>
#include "states/splash_screen/state_splash_screen.hpp"
#include "states/context.hpp"
#include "managers/tick_manager.hpp"
#include "camera.hpp"
#include "tyra"

using Tyra::Engine;

class StateManager {
 public:
  StateManager(Engine* t_engine, Camera* t_camera);
  ~StateManager();

  void update(const float& deltaTime);

  Context* context;
  TickManager tickManager;
};
