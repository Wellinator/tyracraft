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

  inline const u8 isSongPlaying() {
    return creativeAudioListener.t_song->isPlaying();
  };

  /**
   * @brief Print RAM memory info to log
   *
   */
  void printMemoryInfoToLog();

  PostFxManager postFxManager;

  CreativeAudioListener creativeAudioListener;
  u32 audioListenerId;
  float elapsedTimeInSec;
  TickManager tickManager;
  TickTaskHandles tickHandles;

  Vec4 playerMovementDirection;

  inline const u8 isInventoryOpened();

  std::string Message_Saved_Successfully =
      LanguageManager::Translate("/state_game_menu/saved_successfully");
  std::string Message_Progress_Has_Been_Saved =
      LanguageManager::Translate("/state_game_menu/progress_saved");
};
