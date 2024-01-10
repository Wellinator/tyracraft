#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include <tamtypes.h>
#include <tyra>

class SurvivalPlayingState : public PlayingStateBase {
 public:
  SurvivalPlayingState(StateGamePlay* t_context);
  ~SurvivalPlayingState();

  void init();
  void update(const float& deltaTime);
  void tick();
  void render();

 private:
  void handleInput(const float& deltaTime);
  void navigate();
};
