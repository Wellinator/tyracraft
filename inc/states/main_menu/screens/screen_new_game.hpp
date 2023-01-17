#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include "models/new_game_model.hpp"
#include "models/texture_pack_info_model.hpp"
#include "managers/font/font_options.hpp"
#include <tamtypes.h>
#include <tyra>
#include <string>
#include <fstream>
#include <3libs/nlohmann/json.hpp>

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Math;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::TextureRepository;

enum class ScreenNewGameOptions { Seed, WorldType, CreateNewWorld, None };

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

  Sprite slotSeed;
  Sprite slotSeedActive;
  Sprite slotSeedInput;

  Sprite slotWorldType;
  Sprite slotWorldTypeActive;

  Sprite slotCreateNewWorld;
  Sprite slotCreateNewWorldActive;

  Sprite btnTriangle;
  Sprite btnCross;
  Sprite btnCircle;

  Texture* slotTexture;
  Texture* slotActiveTexture;

  ScreenNewGameOptions selectedOption = ScreenNewGameOptions::None;
  ScreenNewGameOptions activeOption = ScreenNewGameOptions::CreateNewWorld;

  NewGameOptions model = NewGameOptions();

  u8 fpsCounter = 0;

  std::string inputSeed;
  std::string tempSeed;
  std::string tempSeedMask;
  u8 isEditingSeed = false;
  u8 editingIndex = 0;

  std::vector<TexturePackInfoModel*> texturePacks;
  TexturePackInfoModel* selectedTexturePack;

  void handleInput();
  void handleOptionsSelection();
  void handleSeedInput();
  void updateTempSeedMask();
  void backToMainMenu();
  void createNewWorld();
  void updateModel();
  void renderSelectedOptions();
  void saveSeed();
  void startEditingSeed();
  void cancelEditingSeed();
  std::string getSeed();

  void getAvailableTexturePacks();
};
