#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "states/game_play/creative_audio_listener.hpp"
#include <tamtypes.h>
#include <tyra>

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

  CreativeAudioListener* t_creativeAudioListener;
};
