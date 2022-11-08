#pragma once
#include "states/game_state.hpp"
#include "managers/sound_manager.hpp"
#include "camera.hpp"
#include <tyra>

using Tyra::Engine;

class GameState;

class Context {
 public:
  Context(Engine* t_engine, Camera* t_camera);
  ~Context();

  void update(const float& deltaTime);
  void setState(GameState* newState);

  Camera* t_camera = nullptr;
  SoundManager* t_soundManager = nullptr;
  Engine* t_engine = nullptr;

 private:
  GameState* state = nullptr;
};
