#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "models/new_game_model.hpp"
#include "models/texture_pack_info_model.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/font/font_options.hpp"
#include "managers/language_manager.hpp"
#include <tamtypes.h>
#include <tyra>
#include <string>
#include <vector>
#include <fstream>
#include <3libs/nlohmann/json.hpp>
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

enum class ScreenNewGameOptions {
  TexturePack,
  WorldName,
  Seed,
  WorldType,
  CreateNewWorld,
  None
};

class ScreenNewGame : public ScreenBase {
 public:
  ScreenNewGame(StateMainMenu* t_context);
  ~ScreenNewGame();

  void init();
  void update(const float& deltaTime);
  void render();

 private:
  Renderer* t_renderer;
  FontManager* pFontManager;

  Sprite backgroundNewGame;
  Sprite tab2;

  Sprite slotWorldName;
  Sprite slotWorldNameActive;
  Sprite slotWorldNameInput;
  Sprite slotSeed;
  Sprite slotSeedActive;
  Sprite slotSeedInput;
  Sprite slotTextureActive;

  Sprite slotWorldType;
  Sprite slotWorldTypeActive;

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

  ScreenNewGameOptions selectedOption = ScreenNewGameOptions::None;
  ScreenNewGameOptions activeOption = ScreenNewGameOptions::CreateNewWorld;

  NewGameOptions model = NewGameOptions();

  u8 fpsCounter = 0;
  const float slotWidth = 246.0F;
  const float slotHeight = 32.0F;

  const u8 MAX_WORLD_NAME_LENGTH = 17;
  const u8 MIN_WORLD_NAME_LENGTH = 1;

  std::string inputSeed;
  std::string tempSeed;
  std::string tempSeedMask;
  u8 isEditingSeed = false;
  u8 editingIndex = 0;

  char tempNewChar = ' ';
  std::string inputWorldName = "New World";
  std::string tempWorldName;
  std::string tempWorldNameMask;
  u8 isEditingWorldName = false;
  u8 editingIndexWorldName = 0;
  u8 needToChangeWorldName = 0;

  const std::string Label_Load      = LanguageManager::Translate("/gui/load");
  const std::string Label_Create    = LanguageManager::Translate("/gui/create");
  const std::string Label_Mini      = LanguageManager::Translate("/gui/mini");
  const std::string Label_Confirm   = LanguageManager::Translate("/gui/confirm");
  const std::string Label_Cancel    = LanguageManager::Translate("/gui/cancel");
  const std::string Label_Select    = LanguageManager::Translate("/gui/select");
  const std::string Label_Bksp      = LanguageManager::Translate("/gui/bksp");
  const std::string Label_Prev      = LanguageManager::Translate("/gui/prev");
  const std::string Label_Next      = LanguageManager::Translate("/gui/next");
  const std::string Label_Back      = LanguageManager::Translate("/gui/back");
  const std::string Label_Edit      = LanguageManager::Translate("/gui/edit");
  const std::string Label_Random    = LanguageManager::Translate("/gui/random");

  const std::string Label_By    = LanguageManager::Translate("/common/by");
  const std::string Label_Seed  = LanguageManager::Translate("/common/seed");
  const std::string Label_Ops   = LanguageManager::Translate("/common/ops_exclamation");

  const std::string Label_TexturePack           = LanguageManager::Translate("/screen_new_game/texture_packs");
  const std::string Label_CreateNewWorld        = LanguageManager::Translate("/screen_new_game/create_new_world");
  const std::string Label_WorldTypeOriginal     = LanguageManager::Translate("/screen_new_game/world_type_original");
  const std::string Label_WorldTypeFlat         = LanguageManager::Translate("/screen_new_game/world_type_flat");
  const std::string Label_WorldTypeIsland       = LanguageManager::Translate("/screen_new_game/world_type_island");
  const std::string Label_WorldTypeWoods        = LanguageManager::Translate("/screen_new_game/world_type_woods");
  const std::string Label_WorldTypeFloating     = LanguageManager::Translate("/screen_new_game/world_type_floating");
  const std::string Label_WorldNameErrorPart1   = LanguageManager::Translate("/screen_new_game/world_name_exists_error_part1");
  const std::string Label_WorldNameErrorPart2   = LanguageManager::Translate("/screen_new_game/world_name_exists_error_part2");
  const std::string Label_WorldNameErrorPart3   = LanguageManager::Translate("/screen_new_game/world_name_exists_error_part3");

  std::vector<TexturePackInfoModel*> texturePacks;
  TexturePackInfoModel* selectedTexturePack = nullptr;

  std::string getSeed();
  void handleInput();
  void handleOptionsSelection();
  void handleSeedInput();
  void handleWorldNameInput();
  void backToMainMenu();
  void createNewWorld();
  bool canCreateANewWorldWithCurrentName();
  void updateModel();
  void renderSelectedOptions();
  void renderWorldNameDialog();
  void saveSeed();
  void startEditingSeed();
  void cancelEditingSeed();
  void updateTempSeedMask();
  void saveWorldName();
  void startEditingWorldName();
  void cancelEditingWorldName();
  void updateTempWorldNameMask();
  void updateTempWorldNameMaskCursor();
  void addWorldNameLastChar();
  void removeWorldNameLastChar();
  void selectPreviousTexturePack();
  void selectNextTexturePack();
  void getAvailableTexturePacks();
};
