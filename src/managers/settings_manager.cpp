#include "managers/settings_manager.hpp"

/**
 * Definition global settings variable
 * g_settings - settings object
 * g_settings_path - path of settings object
 */
settings_file g_settings = settings_file();
const std::string g_settings_path = "config.ini";

SettingsManager::SettingsManager() {}

SettingsManager::~SettingsManager() {}

void SettingsManager::Save(settings_file settings) {
  TYRA_LOG("Saving settings...");

  ini::IniFile _settings;
  g_settings = settings;

  _settings["Graphics"]["vsync"] = settings.vsync;
  _settings["Language"]["language"] = settings.language;
  _settings["DeadZoneOffset"]["l_stick_H"] = settings.l_stick_H;
  _settings["DeadZoneOffset"]["l_stick_V"] = settings.l_stick_V;
  _settings["DeadZoneOffset"]["r_stick_H"] = settings.r_stick_H;
  _settings["DeadZoneOffset"]["r_stick_V"] = settings.r_stick_V;
  _settings["Camera"]["invert_cam_y"] = settings.invert_cam_y;
  _settings["Camera"]["cam_h_sensitivity"] = settings.cam_h_sensitivity;
  _settings["Camera"]["cam_v_sensitivity"] = settings.cam_v_sensitivity;
  _settings["Player"]["skin"] = settings.skin;

  std::ofstream os(FileUtils::fromCwd(g_settings_path));
  _settings.encode(os);
};

void SettingsManager::Save() {
  TYRA_LOG("Saving settings...");
  ini::IniFile _settings;

  _settings["Graphics"]["vsync"] = g_settings.vsync;
  _settings["Language"]["language"] = g_settings.language;
  _settings["DeadZoneOffset"]["l_stick_H"] = g_settings.l_stick_H;
  _settings["DeadZoneOffset"]["l_stick_V"] = g_settings.l_stick_V;
  _settings["DeadZoneOffset"]["r_stick_H"] = g_settings.r_stick_H;
  _settings["DeadZoneOffset"]["r_stick_V"] = g_settings.r_stick_V;
  _settings["Camera"]["invert_cam_y"] = g_settings.invert_cam_y;
  _settings["Camera"]["cam_h_sensitivity"] = g_settings.cam_h_sensitivity;
  _settings["Camera"]["cam_v_sensitivity"] = g_settings.cam_v_sensitivity;
  _settings["Player"]["skin"] = g_settings.skin;

  std::ofstream os(FileUtils::fromCwd(g_settings_path));
  _settings.encode(os);
};

settings_file SettingsManager::Load() {
  TYRA_LOG("Loading settings...");

  std::ifstream saveFile(FileUtils::fromCwd(g_settings_path));
  ini::IniFile _settings(saveFile);

  g_settings.vsync = _settings["Graphics"]["vsync"].as<bool>();
  g_settings.language = _settings["Language"]["language"].as<std::string>();
  g_settings.l_stick_H = _settings["DeadZoneOffset"]["l_stick_H"].as<float>();
  g_settings.l_stick_V = _settings["DeadZoneOffset"]["l_stick_V"].as<float>();
  g_settings.r_stick_H = _settings["DeadZoneOffset"]["r_stick_H"].as<float>();
  g_settings.r_stick_V = _settings["DeadZoneOffset"]["r_stick_V"].as<float>();
  g_settings.invert_cam_y = _settings["Camera"]["invert_cam_y"].as<bool>();
  g_settings.cam_h_sensitivity =
      _settings["Camera"]["cam_h_sensitivity"].as<int>();
  g_settings.cam_v_sensitivity =
      _settings["Camera"]["cam_v_sensitivity"].as<int>();

  // Selected skin name
  const auto skinName = _settings["Player"]["skin"].as<std::string>();
  if (skinName.empty()) {
    g_settings.skin = std::string("steve");
  } else {
    g_settings.skin = skinName;
  }

  return g_settings;
};

bool SettingsManager::CheckIfSettingsExist() {
  struct stat buffer;
  return (stat(FileUtils::fromCwd(g_settings_path).c_str(), &buffer) == 0);
}

void SettingsManager::ApplyChanges(Engine* t_engine) {
  t_engine->renderer.setFrameLimit(g_settings.vsync);
}