#pragma once
#include "states/game_state.hpp"
#include "managers/sound_manager.hpp"
#include "managers/font/font_manager.hpp"
#include "camera.hpp"
#include <tyra>

using Tyra::Engine;

class GameState;

class Context {
 public:
  Context(Engine* t_engine, Camera* t_camera);
  ~Context();

  void fixedUpdate(const float& fixedDeltaTime);
  void update(const float& deltaTime);
  void render();
  void setState(GameState* newState);

  SoundManager soundManager;
  FontManager* pFontManager;
  Camera* t_camera = nullptr;
  Engine* t_engine = nullptr;

 private:
  GameState* state = nullptr;
};
