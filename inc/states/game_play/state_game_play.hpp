#pragma once
#include <debug/debug.hpp>
#include <tamtypes.h>
#include <sifrpc.h>
#include <debug.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include "constants.hpp"
#include "camera.hpp"
#include "states/game_state.hpp"
#include "states/context.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include "states/game_play/states/creative/creative_playing_state.hpp"
#include "states/game_play/states/survival/survival_playing_state.hpp"
#include <tyra>
#include "entities/World.hpp"
#include "entities/player/player.hpp"
#include "managers/items_repository.hpp"
#include "ui.hpp"
#include <chrono>

using Tyra::Audio;
using Tyra::Mesh;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Info;
using Tyra::StaticMesh;

enum class GameMode { Survival, Creative };

class PlayingStateBase;

class StateGamePlay : public GameState {
 public:
  StateGamePlay(Context* context, const GameMode& gameMode);
  ~StateGamePlay();

  void init();
  void update(const float& deltaTime);
  void render();

  void inline backToGame() { this->unpauseGame(); };
  void quitToTitle();

  void setPlayingState(PlayingStateBase* t_playingState);
  PlayingStateBase* getPreviousState();

  // Rotating skybox
  StaticMesh* menuSkybox;

  World* world;
  Ui* ui;
  Player* player;
  ItemRepository* itemRepository;

 private:
  PlayingStateBase* state = nullptr;
  PlayingStateBase* previousState = nullptr;
  u8 paused = false;

  void handleGameMode(const GameMode& gameMode);
  void handleInput();
  void pauseGame();
  void unpauseGame();
};
