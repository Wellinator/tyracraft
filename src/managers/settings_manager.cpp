#include "managers/settings_manager.hpp"

static int clampInt(const int value, const int minValue, const int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

static float clampFloat(const float value, const float minValue,
                        const float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

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

  _settings["Graphics"]["fps_mode"] = static_cast<int>(settings.fps_mode);
  _settings["Language"]["language"] = settings.language;
  _settings["DeadZoneOffset"]["l_stick_H"] = settings.l_stick_H;
  _settings["DeadZoneOffset"]["l_stick_V"] = settings.l_stick_V;
  _settings["DeadZoneOffset"]["r_stick_H"] = settings.r_stick_H;
  _settings["DeadZoneOffset"]["r_stick_V"] = settings.r_stick_V;
  _settings["Camera"]["invert_cam_y"] = settings.invert_cam_y;
  _settings["Camera"]["cam_h_sensitivity"] = settings.cam_h_sensitivity;
  _settings["Camera"]["cam_v_sensitivity"] = settings.cam_v_sensitivity;
  _settings["Player"]["skin"] = settings.skin;
  _settings["AutoSave"]["interval"] = settings.auto_save_interval;


  std::ofstream os(FileUtils::fromCwd(g_settings_path));
  _settings.encode(os);
};

void SettingsManager::Save() {
  TYRA_LOG("Saving settings...");
  ini::IniFile _settings;

  _settings["Graphics"]["fps_mode"] = static_cast<int>(g_settings.fps_mode);
  _settings["Language"]["language"] = g_settings.language;
  _settings["DeadZoneOffset"]["l_stick_H"] = g_settings.l_stick_H;
  _settings["DeadZoneOffset"]["l_stick_V"] = g_settings.l_stick_V;
  _settings["DeadZoneOffset"]["r_stick_H"] = g_settings.r_stick_H;
  _settings["DeadZoneOffset"]["r_stick_V"] = g_settings.r_stick_V;
  _settings["Camera"]["invert_cam_y"] = g_settings.invert_cam_y;
  _settings["Camera"]["cam_h_sensitivity"] = g_settings.cam_h_sensitivity;
  _settings["Camera"]["cam_v_sensitivity"] = g_settings.cam_v_sensitivity;
  _settings["Player"]["skin"] = g_settings.skin;
  _settings["AutoSave"]["interval"] = g_settings.auto_save_interval;


  std::ofstream os(FileUtils::fromCwd(g_settings_path));
  _settings.encode(os);
};

settings_file SettingsManager::Load() {
  TYRA_LOG("Loading settings...");

  std::ifstream saveFile(FileUtils::fromCwd(g_settings_path));
  ini::IniFile _settings(saveFile);

  // fps_mode stored as int; default FPS_60 if key missing
  try {
    int mode = _settings["Graphics"]["fps_mode"].as<int>();
    if (mode < 0 || mode > 2) mode = static_cast<int>(FpsMode::FPS_60);
    g_settings.fps_mode = static_cast<FpsMode>(mode);
  } catch (...) {
    g_settings.fps_mode = FpsMode::FPS_60;
  }
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

  // Auto-save interval (minimum 30 seconds)
  try {
    float interval = _settings["AutoSave"]["interval"].as<float>();
    g_settings.auto_save_interval = (interval >= 30.0f) ? interval : 60.0f;
  } catch (...) {
    g_settings.auto_save_interval = 60.0f;
  }



  return g_settings;
};

bool SettingsManager::CheckIfSettingsExist() {
  struct stat buffer;
  return (stat(FileUtils::fromCwd(g_settings_path).c_str(), &buffer) == 0);
}

void SettingsManager::ApplyChanges(Engine* t_engine) {
  // Test the new VSync setting
  // t_engine->renderer.setFrameLimit(g_settings.vsync);
}