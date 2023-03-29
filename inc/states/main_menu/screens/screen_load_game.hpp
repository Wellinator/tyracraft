#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "models/new_game_model.hpp"
#include "models/save_info_model.hpp"
#include "managers/font/font_options.hpp"
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

  std::array<Sprite, 3> saveFiles;

  u8 currentSaveIndex = 0;
  u8 totalOfSaves = 0;
  const u8 MAX_ITEMS_PER_PAGE = 4;

  Texture* slotTexture;
  Texture* slotActiveTexture;

  ScreenLoadGameOptions activeOption = ScreenLoadGameOptions::SaveFile;
  ScreenLoadGameSaveOrigin saveOrigin = ScreenLoadGameSaveOrigin::Mass;

  std::vector<SaveInfoModel*> savedGamesList;
  SaveInfoModel* selectedSavedGame = nullptr;

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
