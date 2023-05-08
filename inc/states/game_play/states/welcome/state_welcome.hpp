#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "managers/font/font_manager.hpp"
#include "constants.hpp"
#include "entities/World.hpp"
#include <string>
#include <tamtypes.h>
#include <tyra>

using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::TextureRepository;

class StateWelcome : public PlayingStateBase {
 public:
  StateWelcome(StateGamePlay* t_context);
  ~StateWelcome();

  void init();
  void update(const float& deltaTime);
  void render();
  void handleInput(const float& deltaTime);

  void playClickSound();

 private:
  Renderer* t_renderer;

  Sprite overlay;
  Sprite btnCross;

  void unloadTextures();
};
