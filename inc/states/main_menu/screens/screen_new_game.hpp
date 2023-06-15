#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "models/new_game_model.hpp"
#include "models/texture_pack_info_model.hpp"
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
  void update();
  void render();

 private:
  Renderer* t_renderer;

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

  const std::string Label_Load =
      g_language_repository["gui"]["load"].get<std::string>();
  const std::string Label_Create =
      g_language_repository["gui"]["create"].get<std::string>();
  const std::string Label_Confirm =
      g_language_repository["gui"]["confirm"].get<std::string>();
  const std::string Label_Cancel =
      g_language_repository["gui"]["cancel"].get<std::string>();
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

  const std::string Label_By =
      g_language_repository["common"]["by"].get<std::string>();
  const std::string Label_Seed =
      g_language_repository["common"]["seed"].get<std::string>();
  const std::string Label_Ops =
      g_language_repository["common"]["ops_exclamation"].get<std::string>();

  const std::string Label_TexturePack =
      g_language_repository["screen_new_game"]["texture_packs"]
          .get<std::string>();
  const std::string Label_CreateNewWorld =
      g_language_repository["screen_new_game"]["create_new_world"]
          .get<std::string>();
  const std::string Label_WorldTypeOriginal =
      g_language_repository["screen_new_game"]["world_type_original"]
          .get<std::string>();
  const std::string Label_WorldTypeFlat =
      g_language_repository["screen_new_game"]["world_type_flat"]
          .get<std::string>();
  const std::string Label_WorldTypeIsland =
      g_language_repository["screen_new_game"]["world_type_island"]
          .get<std::string>();
  const std::string Label_WorldTypeWoods =
      g_language_repository["screen_new_game"]["world_type_woods"]
          .get<std::string>();
  const std::string Label_WorldTypeFloating =
      g_language_repository["screen_new_game"]["world_type_floating"]
          .get<std::string>();
  const std::string Label_WorldNameErrorPart1 =
      g_language_repository["screen_new_game"]["world_name_exists_error_part1"]
          .get<std::string>();
  const std::string Label_WorldNameErrorPart2 =
      g_language_repository["screen_new_game"]["world_name_exists_error_part2"]
          .get<std::string>();
  const std::string Label_WorldNameErrorPart3 =
      g_language_repository["screen_new_game"]["world_name_exists_error_part3"]
          .get<std::string>();

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
