#pragma once
#include <tyra>
#include <file/file_utils.hpp>
#include <vector>
#include <string>
#include <fstream>
#include "constants.hpp"
#include "states/game_state.hpp"
#include "states/context.hpp"
#include "models/language_info_model.hpp"
#include <managers/language_manager.hpp>
#include <managers/settings_manager.hpp>
#include "3libs/nlohmann/json.hpp"
#include "states/language_selection/language_selection_screen.hpp"
#include "states/main_menu/state_main_menu.hpp"

using Tyra::FileUtils;

class StateLanguageLoaderScreen : public GameState {
 public:
  StateLanguageLoaderScreen(Context* context) : GameState(context){};

  ~StateLanguageLoaderScreen(){};

  void init(){};
  
  void afterInit(){};

  void update(const float& deltaTime) {
    if (SettingsManager::CheckIfSettingsExist()) {
      loadSavedLanguage();
      context->setState(new StateMainMenu(context));
    } else {
      context->setState(new StateLanguageSelectionScreen(context));
    }
  };
  void render(){};

 private:
  void loadSavedLanguage() {
    std::string savedLangPath = std::string("lang/")
                                    .append(g_settings.language)
                                    .append("/language.json");

    std::ifstream langInfo(FileUtils::fromCwd(savedLangPath));
    if (langInfo.is_open()) {
      nlohmann::json language = nlohmann::json::parse(langInfo);
      LanguageManager_SetLanguage(language);
      langInfo.close();
    }
  };
};
