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

enum class GameMenuOptions {
  DrawDistance,
  SaveGame,
  SaveAndQuit,
  QuitWithoutSave,
  None
};

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

  Texture* textureRawSlot;
  Sprite overlay;
  Sprite raw_slot[3];
  Sprite active_slot;
  Sprite btnCross;
  Sprite btnTriangle;
  Sprite horizontalScrollArea;
  Sprite horizontalScrollHandler;
  Sprite dialogWindow;
  Sprite btnStart;

  GameMenuOptions selectedOption = GameMenuOptions::None;
  GameMenuOptions activeOption = GameMenuOptions::SaveGame;

  u8 needSaveOverwriteConfirmation = false;
  u8 needSaveAndQuitConfirmation = false;
  u8 needQuitWithoutSaveConfirmation = false;

  const float SLOT_WIDTH = 230;
  const float SLOT_HIGHT_OFFSET = 240;
  const float SLOT_HIGHT_OPTION_OFFSET = 40;
  const u8 MENU_SFX_CH = 1;

  void hightLightActiveOption();
  void navigate();
  void unloadTextures();
  void increaseDrawDistance();
  void decreaseDrawDistance();
  void updateDrawDistanceScroll();
  void renderSaveOverwritingDialog();
  void renderSaveAndQuitDialog();
  void renderQuitWithoutSavingDialog();
};
