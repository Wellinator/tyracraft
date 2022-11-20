#pragma once

#include <debug/debug.hpp>
#include <tyra>
#include <tamtypes.h>
#include <sifrpc.h>
#include <debug.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include "constants.hpp"
#include "camera.hpp"
#include "renderer/3d/pipeline/dynamic/dynamic_pipeline.hpp"
#include "states/game_state.hpp"
#include "states/context.hpp"
#include "states/game_play/state_game_play.hpp"
#include "entities/World.hpp"
#include "entities/player.hpp"
#include "managers/items_repository.hpp"
#include "ui.hpp"
#include "thread"
#include "models/new_game_model.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Sprite;
using Tyra::Vec4;

enum class LoadingState { Loading, Complete };

class StateLoadingGame : public GameState {
 public:
  StateLoadingGame(Context* context, const NewGameOptions& options);
  ~StateLoadingGame();

  void init();
  void update(const float& deltaTime);
  void render();

 private:
  u8 shouldCreatedEntities = 1;
  u8 shouldInitWorld = 1;
  u8 shouldInitItemRepository = 1;
  u8 shouldInitUI = 1;
  u8 shouldInitPlayer = 1;

  Sprite* background;
  Sprite* loadingSlot;
  Sprite* loadingprogress;
  Sprite* loadingStateText;
  LoadingState _state = LoadingState::Loading;
  float _percent = 1.0F;
  float BASE_HEIGHT;

  StateGamePlay* stateGamePlay = nullptr;
  NewGameOptions worldOptions;

  void setPercent(float completed);
  void setBgColorBlack();

  void createEntities();
  void initItemRepository();
  void initUI();
  void initWorld();
  void initPlayer();

  void nextState();
  void unload();
  bool hasFinished();
};
