#pragma once
#include <tyra>
#include <file/file_utils.hpp>
#include <vector>
#include "constants.hpp"
#include "states/game_state.hpp"
#include "states/context.hpp"
#include "models/language_info_model.hpp"
#include "3libs/nlohmann/json.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Sprite;
using Tyra::TextureRepository;

class StateLanguageSelectionScreen : public GameState {
 public:
  StateLanguageSelectionScreen(Context* context);
  ~StateLanguageSelectionScreen();

  void init();
  void afterInit(){};
  void update(const float& deltaTime);
  void render();

 private:
  Renderer* t_renderer;

  const float WIDTH = 512.0F;
  const float HEIGHT = 448.0F;
  const float FlagIconWidth = 256.0F;
  const float FlagIconHeight = 256.0F;

  std::vector<LanguageInfoModel> languages;
  LanguageInfoModel* selectedLanguage = nullptr;

  Color infoColor = Color(200, 200, 200);

  Sprite background;
  Sprite btnCross;
  Sprite arrowLeft;
  Sprite arrowRight;

  void unloadTextures();
  void nextState();
  void loadLanguagesAvailable();
  void loadSavedLanguage();
  void selectLanguage();
  void selectPreviousLanguage();
  void selectNextLanguage();
};
