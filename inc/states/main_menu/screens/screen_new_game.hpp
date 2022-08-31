#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include "models/new_game_model.hpp"
#include <tamtypes.h>
#include <tyra>

using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;

enum class ScreenNewGameOptions {
  FlatWorld,
  EnableTrees,
  EnableWater,
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

  Sprite checkboxUnfilledFlatWorld;
  Sprite checkboxUnfilledEnableTrees;
  Sprite checkboxUnfilledEnableWater;
  Sprite checkboxFilledFlatWorld;
  Sprite checkboxFilledEnableTrees;
  Sprite checkboxFilledEnableWater;

  Sprite textFlatWorld;
  Sprite textEnableTrees;
  Sprite textEnableWater;
  Sprite textCreateNewWorld;

  Sprite slotCreateNewWorld;
  Sprite slotCreateNewWorldActive;

  Sprite textBack;
  Sprite textSelect;
  Sprite btnTriangle;
  Sprite btnCross;

  Texture* checkboxUnfilledWhiteBorderTexture;
  Texture* checkboxFilledWhiteBorderTexture;

  ScreenNewGameOptions selectedOption = ScreenNewGameOptions::None;
  ScreenNewGameOptions activeOption = ScreenNewGameOptions::CreateNewWorld;

  NewGameOptions model;

  const float CHECKBOX_WIDTH = 160;
  const float CHECKBOX_HIGHT_OFFSET = 240;
  const float CHECKBOX_HIGHT_OPTION_OFFSET = 40;

  void handleInput();
  void backToMainMenu();
  void createNewWorld();
  void hightLightActiveOption();
  void updateModel();
  void renderSelectedOptions();
};
