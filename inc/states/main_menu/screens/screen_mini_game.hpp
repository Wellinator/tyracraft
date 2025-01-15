#pragma once

#include "states/main_menu/screens/screen_base.hpp"
#include "models/new_game_model.hpp"
#include "models/save_info_model.hpp"
#include "models/mini_game_info_model.hpp"
#include "managers/font/font_manager.hpp"
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
  FontManager* pFontManager;

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
  u8 displayProgressReseted = false;

  const std::string Label_Load                                  = LanguageManager::Translate("/gui/load");
  const std::string Label_Create                                = LanguageManager::Translate("/gui/create");
  const std::string Label_Mini                                  = LanguageManager::Translate("/gui/mini");
  const std::string Label_Confirm                               = LanguageManager::Translate("/gui/confirm");
  const std::string Label_Select                                = LanguageManager::Translate("/gui/select");
  const std::string Label_Bksp                                  = LanguageManager::Translate("/gui/bksp");
  const std::string Label_Prev                                  = LanguageManager::Translate("/gui/prev");
  const std::string Label_Next                                  = LanguageManager::Translate("/gui/next");
  const std::string Label_Back                                  = LanguageManager::Translate("/gui/back");
  const std::string Label_Edit                                  = LanguageManager::Translate("/gui/edit");
  const std::string Label_Random                                = LanguageManager::Translate("/gui/random");
  const std::string Label_NewGame                               = LanguageManager::Translate("/minigame/common/new_game");
  const std::string Label_Continue                              = LanguageManager::Translate("/minigame/common/continue");
  const std::string Label_ResetProgress                         = LanguageManager::Translate("/minigame/common/reset_progress");
  const std::string Label_Ops                                   = LanguageManager::Translate("/common/ops_exclamation");
  const std::string Label_Success                               = LanguageManager::Translate("/common/success_exclamation");
  const std::string Label_MiniGame                              = LanguageManager::Translate("/minigame/common/mini_games");
  const std::string Label_PreviousSaveDeletedSuccessPart1       = LanguageManager::Translate("/minigame/common/minigame_previous_save_deleted_part1");
  const std::string Label_PreviousSaveDeletedSuccessPart2       = LanguageManager::Translate("/minigame/common/minigame_previous_save_deleted_part2");
  const std::string Label_PreviousSavePresentErrorPart1         = LanguageManager::Translate("/minigame/common/minigame_previous_save_error_part1");
  const std::string Label_PreviousSavePresentErrorPart2         = LanguageManager::Translate("/minigame/common/minigame_previous_save_error_part2");
  const std::string Label_PreviousSavePresentErrorPart3         = LanguageManager::Translate("/minigame/common/minigame_previous_save_error_part3");

  std::vector<MiniGameInfoModel*> miniGames;
  MiniGameInfoModel* selectedMiniGame = nullptr;
  SaveInfoModel* savedGame = nullptr;

  void handleInput();
  void handleOptionsSelection();
  void backToMainMenu();
  void createNewWorld();
  void deleteProgress();
  void unloadSaved();
  void loadAvailableSaveFromPath();
  bool isThereMiniGameSavedData();
  void updateModel();
  void renderSelectedOptions();
  void renderPreviousSavePresentDialog();
  void renderProgressResetedDialog();
  void selectPreviousMiniGame();
  void selectNextMiniGame();
  void getAvailableMiniGames();
  void loadMazeCraftMiniGameInfo();
};
