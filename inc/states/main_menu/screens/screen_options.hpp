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
  FpsModeOption,
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
  void update(const float& deltaTime);
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

  OptionsScreenOptions activeOption = OptionsScreenOptions::FpsModeOption;

  const float SLOT_WIDTH = 315;
  const float SLOT_HIGHT_OFFSET = 30;
  const float SLOT_HIGHT_OPTION_OFFSET = 40;

  const std::string Label_FpsMode         = LanguageManager::Translate("/options_menu/fps_mode");
  const std::string Label_FpsVSync        = LanguageManager::Translate("/options_menu/fps_vsync");
  const std::string Label_Fps30           = LanguageManager::Translate("/options_menu/fps_30");
  const std::string Label_Fps60           = LanguageManager::Translate("/options_menu/fps_60");
  const std::string Label_FpsUnlimited    = "Unlimited";
  const std::string Label_ReverseCameraY    = LanguageManager::Translate("/options_menu/reverse_camera_y");
  const std::string Label_CamSensitivityH   = LanguageManager::Translate("/options_menu/sensitivity_camera") + "  H: ";
  const std::string Label_CamSensitivityV   = LanguageManager::Translate("/options_menu/sensitivity_camera") + "   V: ";
  const std::string Label_Language          = LanguageManager::Translate("/options_menu/language");
  const std::string Label_DeadZone          = LanguageManager::Translate("/options_menu/deadzone");

  const std::string Label_LStickH           = LanguageManager::Translate("/gui/l_stick") + " " + Label_DeadZone + " H: ";
  const std::string Label_LStickV           = LanguageManager::Translate("/gui/l_stick") + " " + Label_DeadZone + " V: ";
  const std::string Label_RStickH           = LanguageManager::Translate("/gui/r_stick") + " " + Label_DeadZone + " H: ";
  const std::string Label_RStickV           = LanguageManager::Translate("/gui/r_stick") + " " + Label_DeadZone + " V: ";

  const std::string Label_Save              = LanguageManager::Translate("/state_game_menu/save_and_quit");
  const std::string Label_Back              = LanguageManager::Translate("/gui/back");
  const std::string Label_On                = LanguageManager::Translate("/gui/on");
  const std::string Label_Off               = LanguageManager::Translate("/gui/off");

  void hightLightActiveOption();
  void handleInput();
  void navigate();
  void cycleFpsMode(int direction);
  const std::string& getFpsModeLabel() const;
};
