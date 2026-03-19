#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "managers/tick_manager.hpp"
#include "managers/post-fx/post_fx_manager.hpp"
#include <tamtypes.h>
#include <tyra>

class SurvivalPlayingState : public PlayingStateBase {
 public:
  SurvivalPlayingState(StateGamePlay* t_context);
  ~SurvivalPlayingState();

  void init();
  void afterInit();
  void handleAction(MenuAction action){};
  void update(const float& deltaTime);
  void tick();
  void render();
  void processIdleWork();

 private:
  void handleInput(const float& deltaTime);
  void navigate();

  TickManager tickManager;
  TickTaskHandles tickHandles;
  PostFxManager postFxManager;
};
