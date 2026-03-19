#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/language_manager.hpp"
#include "managers/settings_manager.hpp"
#include "constants.hpp"
#include "entities/World.hpp"
#include "timer.hpp"
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
  FpsModeOption,
  SaveGame,
  BackToGame,
  Quit,
  None
};

class StateGameMenu : public PlayingStateBase {
 public:
  StateGameMenu(StateGamePlay* t_context);
  ~StateGameMenu();

  void init();
  void afterInit(){};
  void handleAction(MenuAction action){};
  void update(const float& deltaTime);
  void tick(){};
  void render();
  void processIdleWork();
  void handleInput(const float& deltaTime);

  void playClickSound();

 private:
  Renderer* t_renderer;

  Texture* textureRawSlot;
  Texture* textureRawSlotQuit;
  Sprite overlay;
  // Row 1: half-width slots side by side
  Sprite raw_slot_half[2];
  // Row 2-3: full-width centered slots (Save, Back to Game)
  Sprite raw_slot_full[2];
  // Corner: small quit button
  Sprite raw_slot_quit;
  Sprite active_slot;
  Sprite btnCross;
  Sprite btnTriangle;
  Sprite dialogWindow;
  Sprite background;

  GameMenuOptions selectedOption = GameMenuOptions::None;
  GameMenuOptions activeOption = GameMenuOptions::DrawDistance;

  u8 needSaveOverwriteConfirmation = false;
  u8 needQuitConfirmation = false;

  // Layout constants for 2-column grid
  const float SLOT_HALF_WIDTH = 155.0f;
  const float SLOT_FULL_WIDTH = 320.0f;
  const float SLOT_QUIT_WIDTH = 100.0f;
  const float SLOT_HEIGHT = 35.0f;
  const float SLOT_GAP = 6.0f;
  const float ROW_SPACING = 42.0f;
  const float GRID_START_Y = 200.0f;
  const u8 MENU_SFX_CH = 1;

  // Gui
  const std::string Label_Overwtire =
      LanguageManager::Translate("/gui/overwrite");

  const std::string Label_Quit = LanguageManager::Translate("/gui/quit");
  const std::string Label_Save = LanguageManager::Translate("/gui/save");
  const std::string Label_Select = LanguageManager::Translate("/gui/select");
  const std::string Label_Cancel = LanguageManager::Translate("/gui/cancel");

  // Dialogs
  const std::string Label_DrawDistance                      = LanguageManager::Translate("/state_game_menu/draw_distance");
  const std::string Label_GameMenu                          = LanguageManager::Translate("/state_game_menu/game_menu");
  const std::string Label_BackToGame                        = LanguageManager::Translate("/state_game_menu/back_to_game");
  const std::string Label_OverwriteGameAsk                  = LanguageManager::Translate("/state_game_menu/overwrite_save_game_ask");
  const std::string Label_LocalSaveWillBeOverWriten         = LanguageManager::Translate("/state_game_menu/a_local_save_will_be_overwriten");
  const std::string Label_DoYouWantToContinue               = LanguageManager::Translate("/state_game_menu/do_you_want_to_continue_ask");
  const std::string Label_PreviousSaveErrorMessagePart1     = LanguageManager::Translate("/state_game_menu/previous_save_error_message_part1");
  const std::string Label_PreviousSaveErrorMessagePart2     = LanguageManager::Translate("/state_game_menu/previous_save_error_message_part2");
  const std::string Label_AreYouSure                        = LanguageManager::Translate("/state_game_menu/are_you_sure_ask");
  const std::string Label_AllUnsavedProgressWillBeLost      = LanguageManager::Translate("/state_game_menu/all_unsaved_progress_will_be_lost");

  // Draw distance mode labels
  const std::string Label_ModeAuto   = LanguageManager::Translate("/state_game_menu/draw_distance_auto");
  const std::string Label_ModeLow    = LanguageManager::Translate("/state_game_menu/draw_distance_low");
  const std::string Label_ModeMedium = LanguageManager::Translate("/state_game_menu/draw_distance_medium");
  const std::string Label_ModeHigh   = LanguageManager::Translate("/state_game_menu/draw_distance_high");

  void hightLightActiveOption();
  void navigate();
  void unloadTextures();
  void cycleDrawDistanceMode(int direction);
  const std::string& getDrawDistanceModeLabel() const;
  void cycleFpsMode(int direction);
  const std::string& getFpsModeLabel() const;
  void renderSaveOverwritingDialog();
  void renderSaveAndQuitDialog();
  void renderQuitWithoutSavingDialog();

  // FPS Mode labels
  const std::string Label_FpsMode    = LanguageManager::Translate("/state_game_menu/fps_mode");
  const std::string Label_FpsVSync   = LanguageManager::Translate("/state_game_menu/fps_vsync");
  const std::string Label_Fps30      = LanguageManager::Translate("/state_game_menu/fps_30");
  const std::string Label_Fps60      = LanguageManager::Translate("/state_game_menu/fps_60");
  const std::string Label_FpsUnlimited = LanguageManager::Translate("/state_game_menu/fps_unlimited");
};
