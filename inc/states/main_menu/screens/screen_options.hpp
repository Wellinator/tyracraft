#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include "managers/settings_manager.hpp"
#include <tamtypes.h>
#include <tyra>

using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;

enum class OptionsScreenOptions {
  VSyncOnOff,
  ReverseCamY,
  CamSensitivityH,
  CamSensitivityV,
  ChangeLanguage,
  LStickDeadZoneH,
  LStickDeadZoneV,
  RStickDeadZoneH,
  RStickDeadZoneV,
  None
};

class ScreenOptions : public ScreenBase {
 public:
  ScreenOptions(StateMainMenu* t_context);
  ~ScreenOptions();

  void init();
  void update();
  void render();

 private:
  Renderer* t_renderer;

  // Slots
  Texture* raw_slot_texture;
  Sprite raw_slot[9];
  Sprite active_slot;

  Sprite background;
  Sprite btnTriangle;
  Sprite btnCross;

  settings_file tempSettings = g_settings;

  OptionsScreenOptions activeOption = OptionsScreenOptions::VSyncOnOff;

  const float SLOT_WIDTH = 250;
  const float SLOT_HIGHT_OFFSET = 30;
  const float SLOT_HIGHT_OPTION_OFFSET = 40;

  const std::string Label_UseVsync =
      g_language_repository["options_menu"]["use_vsync"].get<std::string>();
  const std::string Label_ReverseCameraY =
      g_language_repository["options_menu"]["reverse_camera_y"]
          .get<std::string>();
  const std::string Label_CamSensitivityH =
      g_language_repository["options_menu"]["sensitivity_camera"]
          .get<std::string>() +
      " " + std::string(" H: ");
  const std::string Label_CamSensitivityV =
      g_language_repository["options_menu"]["sensitivity_camera"]
          .get<std::string>() +
      " " + std::string(" V: ");
  const std::string Label_Language =
      g_language_repository["options_menu"]["language"].get<std::string>();

  const std::string Label_DeadZone =
      g_language_repository["options_menu"]["deadzone"].get<std::string>();

  const std::string Label_LStickH =
      g_language_repository["gui"]["l_stick"].get<std::string>() + " " +
      Label_DeadZone + std::string(" H: ");
  const std::string Label_LStickV =
      g_language_repository["gui"]["l_stick"].get<std::string>() + " " +
      Label_DeadZone + std::string(" V: ");
  const std::string Label_RStickH =
      g_language_repository["gui"]["r_stick"].get<std::string>() + " " +
      Label_DeadZone + std::string(" H: ");
  const std::string Label_RStickV =
      g_language_repository["gui"]["r_stick"].get<std::string>() + " " +
      Label_DeadZone + std::string(" V: ");

  const std::string Label_Save =
      g_language_repository["state_game_menu"]["save_and_quit"]
          .get<std::string>();
  const std::string Label_Back =
      g_language_repository["gui"]["back"].get<std::string>();
  const std::string Label_On =
      g_language_repository["gui"]["on"].get<std::string>();
  const std::string Label_Off =
      g_language_repository["gui"]["off"].get<std::string>();

  void hightLightActiveOption();
  void handleInput();
  void navigate();
};
