#pragma once

#include <tamtypes.h>
#include "tyra"
#include "constants.hpp"
#include <vector>
#include <string>
#include "models/save_game_model.hpp"
#include <fstream>
#include <3libs/inifile-cpp/inicpp.h>
#include <stdint.h>
#include <sys/stat.h>

using Tyra::FileUtils;

struct settings_file {
  bool vsync = false;
  std::string language = "en_US";
  float l_stick_H = 0.0F;
  float l_stick_V = 0.0F;
  float r_stick_H = 0.0F;
  float r_stick_V = 0.0F;
};

/**
 * Declaration global settings variable
 * g_settings - settings object
 * g_settings_path - path of settings object
 */

extern settings_file g_settings;
extern const std::string g_settings_path;

class SettingsManager {
 public:
  SettingsManager();
  ~SettingsManager();

  static void Save(settings_file settings);
  static void Save();
  static settings_file Load();
  static bool CheckIfSettingsExist();
};