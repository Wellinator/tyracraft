#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "states/game_play/states/creative/creative_audio_listener.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/font/font_options.hpp"
#include "managers/tick_manager.hpp"
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
  void update(const float& deltaTime);
  void render();

 private:
  void handleInput();
  void navigate();
  void renderCreativeUi();
  void drawDegubInfo();

  /**
   * @brief Print blocks info to log
   *
   */
  void printBlockInfoToLog();

  /**
   * @brief Print RAM memory info to log
   *
   */
  void printMemoryInfoToLog();

  CreativeAudioListener* t_creativeAudioListener = nullptr;
  FontManager* t_fontManager = nullptr;
  u32 audioListenerId;
  float elapsedTimeInSec;
  u8 debugMode = false;
};
