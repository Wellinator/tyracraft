#pragma once
#include <tamtypes.h>
#include <pad/pad.hpp>
#include "entities/player.hpp"
#include "entities/World.hpp"
#include "contants.hpp"
#include "ui.hpp"
#include <renderer/3d/mesh/mesh.hpp>
#include "managers/loading_screen.hpp"
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

  World* world;
  Ui* ui;
  Player* player;
  ItemRepository* itemRepository;
  Renderer* t_renderer;
  Audio* t_audio;
  Pad* t_pad;
  Camera* t_camera;

 private:
  // Control game mode change
  // Control Cross click debounce for changing game mode
  std::chrono::steady_clock::time_point lastTimeCrossWasClicked;
  void controlGameMode(Pad& t_pad);
  void loadGame();

  GameState* state = nullptr;
  GAME_MODE gameMode = SURVIVAL;

  LoadingScreen* loadingScreen;
  u8 shouldCreatedEntities = 1;
  u8 shouldInitWorld = 1;
  u8 shouldInitItemRepository = 1;
  u8 shouldInitUI = 1;
  u8 shouldInitPlayer = 1;
};
