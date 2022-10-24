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
#include "states/game_play/states/playing_state_base.hpp"
#include "states/game_play/states/creative/creative_playing_state.hpp"
#include "states/game_play/states/survival/survival_playing_state.hpp"
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
  void setPlayingState(PlayingStateBase* t_playingState);

  // Rotating skybox
  StaticMesh* menuSkybox;

  World* world;
  Ui* ui;
  Player* player;
  ItemRepository* itemRepository;

 private:
  PlayingStateBase* state = nullptr;

  void controlGameMode();
  void handleGameMode(const GameMode& gameMode);
};
