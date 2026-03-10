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
#include "states/game_state.hpp"
#include "states/context.hpp"
#include "states/game_play/state_game_play.hpp"
#include "entities/World.hpp"
#include "entities/player/player.hpp"
#include "managers/items_repository.hpp"
#include "ui.hpp"
#include "thread"
#include "models/save_game_model.hpp"
#include "states/loading/state_loading_game.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Sprite;
using Tyra::Vec4;

class StateLoadMazeCraft : public GameState {
 public:
  StateLoadMazeCraft(Context* context,
                        const std::string save_file_full_path);
  ~StateLoadMazeCraft();

  void init();
  void afterInit(){};
  void update(const float& deltaTime);
  void render();

 private:
  u8 shouldCreatedEntities = 1;
  u8 shouldInitWorld = 1;
  u8 shouldLoadSavedData = 1;
  u8 shouldInitItemRepository = 1;
  u8 shouldInitUI = 1;
  u8 shouldInitPlayer = 1;

  Sprite* background;
  Sprite* loadingSlot;
  Sprite* loadingprogress;

  LoadingState _state = LoadingState::Loading;
  float _percent = 1.0F;
  float BASE_HEIGHT;

  StateGamePlay* stateGamePlay = nullptr;
  NewGameOptions* worldOptions;
  const std::string saveFileFullPath;
  Vec4 savedPlayerPosition;  // Stores player position loaded from save file

  std::string progressLabel = "";
  FontOptions progressLabelOptions =
      FontOptions(Vec2(120.0F, 448.0F - 125.F), Color(200, 200, 200), 0.75F);

  const std::string Label_Loading           = LanguageManager::Translate("/loading_screen/loading");
  const std::string Label_CreatingEntities  = LanguageManager::Translate("/loading_screen/creating_entities");
  const std::string Label_LoadingItemsRepo  = LanguageManager::Translate("/loading_screen/loading_items_repository");
  const std::string Label_LoadingUI         = LanguageManager::Translate("/loading_screen/loading_ui");
  const std::string Label_LoadingWorld      = LanguageManager::Translate("/loading_screen/loading_world");
  const std::string Label_LoadingPlayer     = LanguageManager::Translate("/loading_screen/loading_player");
  const std::string Label_LoadingSave       = LanguageManager::Translate("/loading_screen/loading_save");

  void setPercent(float completed);
  void setBgColorBlack();

  void createEntities();
  void initItemRepository();
  void initUI();
  void initWorld();
  void loadSavedData();
  void initPlayer();

  void nextState();
  void unload();
  bool hasFinished();
};
