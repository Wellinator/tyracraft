#pragma once
#include <tamtypes.h>
#include <pad/pad.hpp>
#include "entities/player.hpp"
#include "entities/World.hpp"
#include "contants.hpp"
#include "ui.hpp"
#include <renderer/3d/mesh/mesh.hpp>
#include "managers/menu_manager.hpp"
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
  Context(Engine* t_engine);
  ~Context();

  void update(const float& deltaTime, Camera* t_camera);

  World* world;
  Ui* ui;
  Player* player;
  ItemRepository* itemRepository;
  Renderer* t_renderer;
  Audio* t_audio;
  Pad* t_pad;

  GameState* state = nullptr;

 private:
  void loadGame();

  GAME_MODE gameMode = SURVIVAL;

  MainMenu* mainMenu;
  LoadingScreen* loadingScreen;
  u8 shouldCreatedEntities = 1;
  u8 shouldInitWorld = 1;
  u8 shouldInitItemRepository = 1;
  u8 shouldInitUI = 1;
  u8 shouldInitPlayer = 1;

  // Control game mode change
  // Control Cross click debounce for changing game mode
  std::chrono::steady_clock::time_point lastTimeCrossWasClicked;
  void controlGameMode(Pad& t_pad);
};
