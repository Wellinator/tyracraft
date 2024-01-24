#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/language_manager.hpp"
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
  void tick(){};
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
  Sprite background;

  GameMenuOptions selectedOption = GameMenuOptions::None;
  GameMenuOptions activeOption = GameMenuOptions::SaveGame;

  u8 needSaveOverwriteConfirmation = false;
  u8 needSaveAndQuitConfirmation = false;
  u8 needQuitWithoutSaveConfirmation = false;

  const float SLOT_WIDTH = 230;
  const float SLOT_HIGHT_OFFSET = 240;
  const float SLOT_HIGHT_OPTION_OFFSET = 40;
  const u8 MENU_SFX_CH = 1;

  // Gui
  const std::string Label_Overwtire =
      g_language_repository["gui"]["overwrite"].get<std::string>();
  const std::string Label_Quit =
      g_language_repository["gui"]["quit"].get<std::string>();
  const std::string Label_Select =
      g_language_repository["gui"]["select"].get<std::string>();
  const std::string Label_Cancel =
      g_language_repository["gui"]["cancel"].get<std::string>();

  // Dialogs
  const std::string Label_DrawDistance =
      g_language_repository["state_game_menu"]["draw_distance"]
          .get<std::string>();
  const std::string Label_GameMenu =
      g_language_repository["state_game_menu"]["game_menu"].get<std::string>();
  const std::string Label_SaveGame =
      g_language_repository["state_game_menu"]["save_game"].get<std::string>();
  const std::string Label_SaveAndQuit =
      g_language_repository["state_game_menu"]["save_and_quit"]
          .get<std::string>();
  const std::string Label_QuitWithoutSave =
      g_language_repository["state_game_menu"]["quit_without_save"]
          .get<std::string>();
  const std::string Label_BackToGame =
      g_language_repository["state_game_menu"]["back_to_game"]
          .get<std::string>();
  const std::string Label_OverwriteGameAsk =
      g_language_repository["state_game_menu"]["overwrite_save_game_ask"]
          .get<std::string>();
  const std::string Label_LocalSaveWillBeOverWriten =
      g_language_repository["state_game_menu"]
                           ["a_local_save_will_be_overwriten"]
                               .get<std::string>();
  const std::string Label_DoYouWantToContinue =
      g_language_repository["state_game_menu"]["do_you_want_to_continue_ask"]
          .get<std::string>();
  const std::string Label_PreviousSaveErrorMessagePart1 =
      g_language_repository["state_game_menu"]
                           ["previous_save_error_message_part1"]
                               .get<std::string>();
  const std::string Label_PreviousSaveErrorMessagePart2 =
      g_language_repository["state_game_menu"]
                           ["previous_save_error_message_part2"]
                               .get<std::string>();
  const std::string Label_AreYouSure =
      g_language_repository["state_game_menu"]["are_you_sure_ask"]
          .get<std::string>();
  const std::string Label_AllUnsavedProgressWillBeLost =
      g_language_repository["state_game_menu"]
                           ["all_unsaved_progress_will_be_lost"]
                               .get<std::string>();

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
