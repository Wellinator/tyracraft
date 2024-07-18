#pragma once

#include "states/main_menu/screens/screen_base.hpp"
#include "models/new_game_model.hpp"
#include "models/mini_game_info_model.hpp"
#include "managers/font/font_options.hpp"
#include "managers/language_manager.hpp"
#include <tamtypes.h>
#include <tyra>
#include <string>
#include <vector>
#include <fstream>
#include <utils.hpp>

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Math;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::TextureRepository;
using json = nlohmann::json;

class StateMainMenu;

enum class ScreenMiniGameOptions {
  MiniGameSelection,
  NewGame,
  LoadGame,
  ResetProgress,
  None
};

enum class MiniGames { MazeCraft };

class ScreenMiniGame : public ScreenBase {
 public:
  ScreenMiniGame(StateMainMenu* t_context);
  ~ScreenMiniGame();

  void init();
  void update(const float& deltaTime);
  void render();

 private:
  Renderer* t_renderer;

  Sprite backgroundNewGame;
  Sprite tab3;

  Sprite slotNewGame;
  Sprite slotNewGameActive;
  Sprite slotLoadGame;
  Sprite slotLoadGameActive;
  Sprite slotTextureActive;

  Sprite slotResetProgress;
  Sprite slotResetProgressActive;

  Sprite slotCreateNewWorld;
  Sprite slotCreateNewWorldActive;

  Sprite btnTriangle;
  Sprite btnTriangleTexturePack;
  Sprite btnCross;
  Sprite btnSquare;
  Sprite btnCircle;
  Sprite btnStart;
  Sprite btnDpadLeft;
  Sprite btnDpadRight;
  Sprite btnL1;
  Sprite btnR1;
  Sprite overlay;
  Sprite dialogWindow;

  Texture* slotTexture;
  Texture* slotActiveTexture;
  Texture* btnTriangleTexture;

  ScreenMiniGameOptions selectedOption = ScreenMiniGameOptions::None;
  ScreenMiniGameOptions activeOption = ScreenMiniGameOptions::MiniGameSelection;

  NewGameOptions model = NewGameOptions();

  //   u8 fpsCounter = 0;
  const float slotWidth = 246.0F;
  const float slotHeight = 32.0F;

  //   const u8 MAX_WORLD_NAME_LENGTH = 17;
  //   const u8 MIN_WORLD_NAME_LENGTH = 1;

  //   std::string inputSeed;
  //   std::string tempSeed;
  //   std::string tempSeedMask;
  //   u8 isEditingSeed = false;
  //   u8 editingIndex = 0;

  char tempNewChar = ' ';
  std::string inputWorldName = "Maze World";
  u8 isEditingWorldName = false;
  u8 editingIndexWorldName = 0;
  u8 displayPreviousSavePresent = false;

  const std::string Label_Load =
      g_language_repository["gui"]["load"].get<std::string>();
  const std::string Label_Create =
      g_language_repository["gui"]["create"].get<std::string>();
  const std::string Label_Mini =
      g_language_repository["gui"]["mini"].get<std::string>();
  const std::string Label_Confirm =
      g_language_repository["gui"]["confirm"].get<std::string>();
  //   const std::string Label_Cancel =
  //       g_language_repository["gui"]["cancel"].get<std::string>();
  const std::string Label_Select =
      g_language_repository["gui"]["select"].get<std::string>();
  const std::string Label_Bksp =
      g_language_repository["gui"]["bksp"].get<std::string>();
  const std::string Label_Prev =
      g_language_repository["gui"]["prev"].get<std::string>();
  const std::string Label_Next =
      g_language_repository["gui"]["next"].get<std::string>();
  const std::string Label_Back =
      g_language_repository["gui"]["back"].get<std::string>();
  const std::string Label_Edit =
      g_language_repository["gui"]["edit"].get<std::string>();
  const std::string Label_Random =
      g_language_repository["gui"]["random"].get<std::string>();

  const std::string Label_NewGame =
      g_language_repository["minigame"]["common"]["new_game"]
          .get<std::string>();
  const std::string Label_Continue =
      g_language_repository["minigame"]["common"]["continue"]
          .get<std::string>();
  const std::string Label_ResetProgress =
      g_language_repository["minigame"]["common"]["reset_progress"]
          .get<std::string>();
  const std::string Label_Ops =
      g_language_repository["common"]["ops_exclamation"].get<std::string>();

  const std::string Label_MiniGame =
      g_language_repository["minigame"]["common"]["mini_games"]
          .get<std::string>();
  //   const std::string Label_CreateNewWorld =
  //       g_language_repository["screen_new_game"]["create_new_world"]
  //   .get<std::string>();
  //   const std::string Label_WorldTypeOriginal =
  //       g_language_repository["screen_new_game"]["world_type_original"]
  //           .get<std::string>();
  //   const std::string Label_WorldTypeFlat =
  //       g_language_repository["screen_new_game"]["world_type_flat"]
  //           .get<std::string>();
  //   const std::string Label_WorldTypeIsland =
  //       g_language_repository["screen_new_game"]["world_type_island"]
  //           .get<std::string>();
  //   const std::string Label_WorldTypeWoods =
  //       g_language_repository["screen_new_game"]["world_type_woods"]
  //           .get<std::string>();
  //   const std::string Label_WorldTypeFloating =
  //       g_language_repository["screen_new_game"]["world_type_floating"]
  //           .get<std::string>();
  //   const std::string Label_WorldTypeMazecraft =
  //       g_language_repository["screen_new_game"]["world_type_mazecraft"]
  //           .get<std::string>();

  const std::string Label_PreviousSavePresentErrorPart1 =
      g_language_repository["minigame"]["common"]
                           ["minigame_previous_save_error_part1"]
                               .get<std::string>();
  const std::string Label_PreviousSavePresentErrorPart2 =
      g_language_repository["minigame"]["common"]
                           ["minigame_previous_save_error_part2"]
                               .get<std::string>();
  const std::string Label_PreviousSavePresentErrorPart3 =
      g_language_repository["minigame"]["common"]
                           ["minigame_previous_save_error_part3"]
                               .get<std::string>();

  std::vector<MiniGameInfoModel*> miniGames;
  MiniGameInfoModel* selectedMiniGame = nullptr;

  void handleInput();
  void handleOptionsSelection();
  void backToMainMenu();
  void createNewWorld();
  bool isThereMiniGameSavedData();
  void updateModel();
  void renderSelectedOptions();
  void renderPreviousSavePresentDialog();
  void selectPreviousMiniGame();
  void selectNextMiniGame();
  void getAvailableMiniGames();
  void loadMazeCraftMiniGameInfo();
};
