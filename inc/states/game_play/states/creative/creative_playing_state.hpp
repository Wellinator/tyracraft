#pragma once

#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "states/game_play/states/creative/creative_audio_listener.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/post-fx/post_fx_manager.hpp"
#include "managers/font/font_options.hpp"
#include "managers/language_manager.hpp"
#include "managers/tick_manager.hpp"
#include "managers/language_manager.hpp"
#include "models/terrain_height_model.hpp"
#include "entities/inventory.hpp"
#include "timer.hpp"
#include "managers/notification/notification.hpp"
#include <tamtypes.h>
#include <string>
#include <tyra>

using Tyra::Pad;
using Tyra::PadButtons;
using Tyra::Threading;

class CreativePlayingState : public PlayingStateBase {
 public:
  CreativePlayingState(StateGamePlay* t_context);
  ~CreativePlayingState();

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
  void renderCreativeUi();
  void drawDebugInfo();
  void playNewRandomSong();
  void openInventory();
  void closeInventory();
  void gamePlayInputHandler(const float& deltaTime);
  void inventoryInputHandler(const float& deltaTime);
  void saveProgress();
  void autoSave();

  inline const u8 isSongPlaying() {
    return creativeAudioListener.t_song->isPlaying();
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

  CreativeAudioListener creativeAudioListener;
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

  inline const u8 isInventoryOpened();

  std::string Message_Saved_Successfully =
      LanguageManager::Translate("/state_game_menu/saved_successfully");
  std::string Message_Progress_Has_Been_Saved =
      LanguageManager::Translate("/state_game_menu/progress_saved");

  // Cached debug strings — updated in update(), read in drawDebugInfo()
  std::string _dbg_seed;
  std::string _dbg_fps;
  std::string _dbg_ticks;
  std::string _dbg_playerPos;
  std::string _dbg_visibleChunks;
  std::string _dbg_chunksToLoad;
  std::string _dbg_chunksToUnload;
  std::string _dbg_chunksToUpdateLight;
  std::string _dbg_particles;
  std::string _dbg_tickAvg;
  std::string _dbg_version;
};
