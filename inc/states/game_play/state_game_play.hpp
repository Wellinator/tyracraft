#pragma once
#include <debug/debug.hpp>
#include <tamtypes.h>
#include <sifrpc.h>
#include <debug.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include "contants.hpp"
#include "camera.hpp"
#include "states/game_state.hpp"
#include "states/context.hpp"
#include <tyra>
#include "entities/World.hpp"
#include "entities/player.hpp"
#include "managers/items_repository.hpp"
#include "ui.hpp"
#include <chrono>

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

  World* world;
  Ui* ui;
  Player* player;
  ItemRepository* itemRepository;

 private:
  // Control game mode change
  // Control Cross click debounce for changing game mode
  std::chrono::steady_clock::time_point lastTimeCrossWasClicked;
  GameMode gameMode = GameMode::Survival;

  void handleInput();
  void controlGameMode();
};
