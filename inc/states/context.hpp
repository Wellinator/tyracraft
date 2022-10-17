#pragma once
#include "states/game_state.hpp"
#include "managers/sound_manager.hpp"
#include "camera.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::Color;
using Tyra::Engine;
using Tyra::Renderer;
using Tyra::Sprite;

class GameState;

class Context {
 public:
  Context(Engine* t_engine, Camera* t_camera);
  ~Context();

  void update(const float& deltaTime);
  void setState(GameState* newState);

  Renderer* t_renderer;
  Audio* t_audio;
  Pad* t_pad;
  Camera* t_camera;
  SoundManager* t_soundManager;

 private:
  GameState* state;
};
