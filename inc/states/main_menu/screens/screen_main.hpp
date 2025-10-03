#pragma once

#include <tamtypes.h>
#include <inttypes.h>
#include <tyra>
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include "managers/font/font_options.hpp"
#include "managers/language_manager.hpp"
#include "managers/settings_manager.hpp"
#include "entities/animation/animated.hpp"
#include "utils.hpp"
#include "models/block_info_model.hpp"
#include "memory.h"

using Tyra::FileUtils;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::StaPipOptions;
using Tyra::StaticPipeline;
using Tyra::Texture;

enum class ScreenMainOptions {
  PlayGame,
  Options,
  HowToPlay,
  About,
  SkinSelection,
  None
};

class ScreenMain : public ScreenBase {
 public:
  ScreenMain(StateMainMenu* t_context);
  ~ScreenMain();

  void init();
  void update(const float& deltaTime);
  void render();
  void renderPlayerPreview();

 private:
  Renderer* t_renderer;

  // Slots
  Texture* raw_slot_texture;
  Sprite raw_slot[4];
  Sprite active_slot;
  Sprite textBack;
  Sprite beacon_button_default;
  Sprite beacon_button_hover;
  Sprite glyph_skin_pack;

  Sprite btnCross;
  Sprite btnTriangle;

  ScreenMainOptions selectedOption = ScreenMainOptions::None;
  ScreenMainOptions activeOption = ScreenMainOptions::PlayGame;

  Animated animator;
  StaPipOptions statPipOptions;
  StaticPipeline statPip;
  const u8 IDLE_ANIMATION = 0;
  std::array<std::unique_ptr<Tyra::Mesh>, 2> animationFrames;
  Texture* skinTexture = nullptr;
  M4x4 playerPreviewModelMatrix;

  void loadSkinTexture(Renderer* renderer);

  const float SLOT_WIDTH = 160;
  const float SLOT_HIGHT_OFFSET = 200;
  const float SLOT_HIGHT_OPTION_OFFSET = 40;

  u8 shouldNavigate = 0;

  void hightLightActiveOption();
  void handleInput();
  void navigate();

  const std::string Label_PlayGame =
      LanguageManager::Translate("/main_menu/play_game");
  const std::string Label_Options =
      LanguageManager::Translate("/main_menu/options");
  const std::string Label_HowToPlay =
      LanguageManager::Translate("/main_menu/how_to_play");
  const std::string Label_About =
      LanguageManager::Translate("/main_menu/about");
  const std::string Label_Select = LanguageManager::Translate("/gui/select");
};
