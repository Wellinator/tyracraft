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
using Tyra::Threading;

enum class GameMenuOptions { DrawDistance, BackToGame, QuitToTitle, None };

class StateGameMenu : public PlayingStateBase {
 public:
  StateGameMenu(StateGamePlay* t_context);
  ~StateGameMenu();

  void init();
  void update(const float& deltaTime);
  void render();
  void handleInput(const float& deltaTime);

  void playClickSound();

 private:
  Renderer* t_renderer;

  // Overlay
  Sprite overlay;

  // Slots
  Texture* textureRawSlot;
  Sprite raw_slot[2];
  Sprite active_slot;

  // Buttons
  Sprite btnCross;

  // Text slot option
  Sprite textGameMenu;
  Sprite textBackToGame;
  Sprite textQuitToTitle;
  Sprite textSelect;

  Sprite horizontalScrollArea;
  Sprite horizontalScrollHandler;

  GameMenuOptions selectedOption = GameMenuOptions::None;
  GameMenuOptions activeOption = GameMenuOptions::BackToGame;

  const float SLOT_WIDTH = 160;
  const float SLOT_HIGHT_OFFSET = 240;
  const float SLOT_HIGHT_OPTION_OFFSET = 40;
  const u8 MENU_SFX_CH = 1;

  void hightLightActiveOption();
  void navigate();
  void unloadTextures();
  void increaseDrawDistance();
  void decreaseDrawDistance();
  void updateDrawDistanceScroll();
};
