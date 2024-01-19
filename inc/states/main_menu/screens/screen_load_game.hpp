#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "models/new_game_model.hpp"
#include "models/save_info_model.hpp"
#include "managers/font/font_options.hpp"
#include "managers/language_manager.hpp"
#include <tamtypes.h>
#include <tyra>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <3libs/nlohmann/json.hpp>
#include <utils.hpp>
#include <stdint.h>

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Math;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::TextureRepository;

enum class ScreenLoadGameOptions { ToggleSaveOrigin, SaveFile, None };
enum class ScreenLoadGameSaveOrigin { Mass, MemoryCard };

class StateMainMenu;

class ScreenLoadGame : public ScreenBase {
 public:
  ScreenLoadGame(StateMainMenu* t_context);
  ~ScreenLoadGame();

  void init();
  void update();
  void render();

 private:
  Renderer* t_renderer;

  Sprite backgroundLoadGame;
  Sprite tab1;

  Sprite toggleBtnOff;
  Sprite toggleBtnOn;

  Sprite btnTriangle;
  Sprite btnCross;
  Sprite btnDpadUp;
  Sprite btnDpadDown;
  Sprite btnSelect;
  Sprite btnL1;
  Sprite btnR1;
  Sprite selectedSaveOverlay;

  std::array<Sprite, 3> saveFiles;

  u8 currentSaveIndex = 0;
  u8 totalOfSaves = 0;
  const u8 MAX_ITEMS_PER_PAGE = 4;

  Texture* saveIconTex;
  Texture* badSaveIconTex;

  ScreenLoadGameOptions activeOption = ScreenLoadGameOptions::SaveFile;
  ScreenLoadGameSaveOrigin saveOrigin = ScreenLoadGameSaveOrigin::Mass;

  std::vector<SaveInfoModel*> savedGamesList;
  SaveInfoModel* selectedSavedGame = nullptr;

  const std::string Label_Load =
      g_language_repository["gui"]["load"].get<std::string>();
  const std::string Label_Create =
      g_language_repository["gui"]["create"].get<std::string>();
  const std::string Label_Prev =
      g_language_repository["gui"]["prev"].get<std::string>();
  const std::string Label_Next =
      g_language_repository["gui"]["next"].get<std::string>();
  const std::string Label_Back =
      g_language_repository["gui"]["back"].get<std::string>();

  void handleInput();
  void handleOptionsSelection();
  void backToMainMenu();
  void toggleSaveOrigin();
  void unloadSaved();
  void loadAvailableSavesFromPath(const char* path);
  void loadAvailableSavesFromUSB();
  void loadAvailableSavesFromMemoryCard();
  void renderSelectedOptions();
  void selectPreviousSave();
  void selectNextSave();
  void loadSelectedSave();
};
