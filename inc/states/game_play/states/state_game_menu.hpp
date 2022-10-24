#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include <tamtypes.h>
#include <tyra>

class StateGameMenu : public PlayingStateBase {
 public:
  StateGameMenu(StateGamePlay* t_context);
  ~StateGameMenu();

  void init();
  void update(const float& deltaTime);
  void render();

 private:
  void handleInput();
  void navigate();
  void unloadTextures();
};
