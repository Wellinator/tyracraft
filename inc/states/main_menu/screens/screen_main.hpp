#pragma once

#include <tamtypes.h>
#include <inttypes.h>
#include <tyra>
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include "managers/font/font_options.hpp"
#include "managers/language_manager.hpp"
#include "utils.hpp"
#include "models/block_info_model.hpp"

using Tyra::DynamicMesh;
using Tyra::DynamicPipeline;
using Tyra::DynPipOptions;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::ObjLoaderOptions;
using Tyra::ObjLoader;

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
  void update();
  void render();

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

  DynPipOptions dynpipOptions;
  DynamicPipeline dynpip;
  std::unique_ptr<DynamicMesh> playerPreviewMesh;
  std::vector<u32> standStillSequence = {0, 1};
  Texture* skinTexture = nullptr;

  void loadSkinTexture(Renderer* renderer);
  void loadPlayerPreview(Renderer* renderer);

  const float SLOT_WIDTH = 160;
  const float SLOT_HIGHT_OFFSET = 200;
  const float SLOT_HIGHT_OPTION_OFFSET = 40;

  void hightLightActiveOption();
  void handleInput();
  void navigate();

  const std::string Label_PlayGame =
      g_language_repository["main_menu"]["play_game"].get<std::string>();
  const std::string Label_Options =
      g_language_repository["main_menu"]["options"].get<std::string>();
  const std::string Label_HowToPlay =
      g_language_repository["main_menu"]["how_to_play"].get<std::string>();
  const std::string Label_About =
      g_language_repository["main_menu"]["about"].get<std::string>();
  const std::string Label_Select =
      g_language_repository["gui"]["select"].get<std::string>();
};
