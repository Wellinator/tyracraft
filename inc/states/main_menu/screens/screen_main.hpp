#pragma once

#include <tamtypes.h>
#include <inttypes.h>
#include <tyra>
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include "managers/font/font_options.hpp"
#include "utils.hpp"
#include "models/block_info_model.hpp"

using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;

enum class ScreenMainOptions { PlayGame, HowToPlay, About, None };

class ScreenMain : public ScreenBase {
 public:
  ScreenMain(StateMainMenu* t_context);
  ~ScreenMain();

  void init();
  void update();
  void render();

 private:
  Renderer* t_renderer;

  // Slots
  Texture* raw_slot_texture;
  Sprite raw_slot[3];
  Sprite active_slot;
  Sprite textBack;

  Sprite btnCross;
  Sprite btnTriangle;

  ScreenMainOptions selectedOption = ScreenMainOptions::None;
  ScreenMainOptions activeOption = ScreenMainOptions::PlayGame;

  const float SLOT_WIDTH = 160;
  const float SLOT_HIGHT_OFFSET = 230;
  const float SLOT_HIGHT_OPTION_OFFSET = 40;

  void hightLightActiveOption();
  void handleInput();
  void navigate();
};
