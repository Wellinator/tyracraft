#pragma once

#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "states/game_play/states/minigame/mazecraft/maze_audio_listener.hpp"
#include "managers/post-fx/post_fx_manager.hpp"
#include "managers/tick_manager.hpp"
#include "models/terrain_height_model.hpp"
#include "entities/inventory.hpp"
#include "timer.hpp"
#include "managers/notification/notification.hpp"
#include <tamtypes.h>
#include <string>
#include <tyra>

using Tyra::Pad;
using Tyra::PadButtons;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::TextureRepository;
using Tyra::Threading;
using Tyra::Vec4;

class MazePlayingState : public PlayingStateBase {
 public:
  MazePlayingState(StateGamePlay* t_context);
  ~MazePlayingState();

  void init();
  void afterInit();
  void handleAction(MenuAction action);
  void fixedUpdate(const float& fixedDeltaTime);
  void update(const float& deltaTime);
  void tick();
  void render();
  void processIdleWork();

 private:
  void handleInput(const float& deltaTime);
  void navigate();
  void renderMazeUi();
  void drawDebugInfo();
  void gamePlayInputHandler(const float& deltaTime);
  void saveProgress();
  void autoSave();
  void loadNextLevel();

  u8 hasReachedTargetBlock();
  void setHappyTheme();
  void setDarkTheme();

  Sprite overlay;
  Sprite btnL2;

  u8 shouldLoadNextLevel = false;
  u8 shouldRenderLevelDoneDialog = false;
  double _nextLevelCounter = 6.0f;
  void renderCountDown();

  const std::string Label_LevelDone =
      LanguageManager::Translate("/minigame/common/level_done_exclamation");
  const std::string Label_LoadingNextLevelIn =
      LanguageManager::Translate("/minigame/common/loading_next_level_in");
  const std::string Label_Interact =
      LanguageManager::Translate("/minigame/common/interact");

  const std::string Message_Saved_Successfully =
      LanguageManager::Translate("/state_game_menu/saved_successfully");
  const std::string Message_Progress_Has_Been_Saved =
      LanguageManager::Translate("/state_game_menu/progress_saved");

  inline const u8 isSongPlaying() {
    return mazeAudioListener.t_song->isPlaying();
  };

  const std::string Label_AutoSaveIn = LanguageManager::Translate("/state_game_menu/auto_save_in");
  const std::string Label_Seconds = LanguageManager::Translate("/state_game_menu/seconds");
  const std::string Label_PressSelectToCancel = LanguageManager::Translate("/state_game_menu/press_select_to_cancel");
  const std::string Label_AutoSaveCancelled = LanguageManager::Translate("/state_game_menu/auto_save_cancelled");
  const std::string Label_SkippedThisAutoSave = LanguageManager::Translate("/state_game_menu/skipped_this_auto_save");
  const std::string Label_AutoSave = LanguageManager::Translate("/state_game_menu/auto_save");
  const std::string Label_Saving = LanguageManager::Translate("/state_game_menu/saving");
  const std::string Label_SavingDoNotTurnOff = LanguageManager::Translate("/state_game_menu/saving_do_not_turn_off");

  /**
   * @brief Print RAM memory info to log
   *
   */
  void printMemoryInfoToLog();

  PostFxManager postFxManager;

  MazeAudioListener mazeAudioListener;
  u32 audioListenerId;
  float elapsedTimeInSec;
  TyraCraft::Timer::ElapsedTimer autoSaveTimer;
  TyraCraft::Timer::ElapsedTimer autoSaveWarningTimer;
  bool isAutoSaveWarningActive = false;
  int autoSaveLastSecondsLeft = 0;
  Notification* autoSaveWarningNotification = nullptr;
  
  TickManager tickManager;
  TickTaskHandles tickHandles;

  Vec4 playerMovementDirection;
};
