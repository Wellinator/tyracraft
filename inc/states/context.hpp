#pragma once
#include <tamtypes.h>
#include <pad/pad.hpp>
#include "entities/player.hpp"
#include "entities/World.hpp"
#include "contants.hpp"
#include "ui.hpp"
#include <renderer/3d/mesh/mesh.hpp>
#include "managers/items_repository.hpp"
#include <chrono>
#include <renderer/renderer.hpp>
#include "renderer/renderer_settings.hpp"
#include "states/game_state.hpp"

using Tyra::Audio;
using Tyra::Pad;
using Tyra::Renderer;
using Tyra::RendererSettings;

class GameState;

class Context {
 public:
  Context(Engine* t_engine, Camera* t_camera);
  ~Context();

  void update(const float& deltaTime);
  void setState(GameState* newState);

  World* world = nullptr;
  Ui* ui = nullptr;
  Player* player = nullptr;
  ItemRepository* itemRepository = nullptr;

  Renderer* t_renderer = nullptr;
  Audio* t_audio = nullptr;
  Pad* t_pad = nullptr;
  Camera* t_camera = nullptr;

 private:
  GameState* state = nullptr;
};
