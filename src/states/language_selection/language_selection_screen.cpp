#include "states/language_selection/language_selection_screen.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include <managers/font/font_options.hpp>
#include <managers/font/font_manager.hpp>
#include <managers/language_manager.hpp>
#include <managers/settings_manager.hpp>
#include <string>
#include <tyra>

StateLanguageSelectionScreen::StateLanguageSelectionScreen(Context* context)
    : GameState(context) {
  t_renderer = &context->t_engine->renderer;
  init();
};

StateLanguageSelectionScreen::~StateLanguageSelectionScreen() {
  unloadTextures();

  languages.clear();
  languages.shrink_to_fit();
};

void StateLanguageSelectionScreen::init() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  background.mode = Tyra::MODE_STRETCH;
  background.size.set(512, 512);
  background.position.set(0, 0);
  textureRepo->add(FileUtils::fromCwd("textures/gui/language/background.png"))
      ->addLink(background.id);

  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15, HEIGHT - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  arrowLeft.mode = Tyra::MODE_STRETCH;
  arrowLeft.size.set(64, 64);
  arrowLeft.color.set(250, 250, 250);
  arrowLeft.position.set(96 - 20, HEIGHT / 2 - 16);
  textureRepo->add(FileUtils::fromCwd("textures/gui/chevron_grey_left.png"))
      ->addLink(arrowLeft.id);

  arrowRight.mode = Tyra::MODE_STRETCH;
  arrowRight.size.set(64, 64);
  arrowRight.color.set(250, 250, 250);
  arrowRight.position.set(416 - 20, HEIGHT / 2 - 16);
  textureRepo->add(FileUtils::fromCwd("textures/gui/chevron_grey_right.png"))
      ->addLink(arrowRight.id);

  loadLanguagesAvailable();
};

void StateLanguageSelectionScreen::update(const float& deltaTime) {
  auto clickedButtons = context->t_engine->pad.getClicked();

  if (clickedButtons.DpadLeft)
    selectPreviousLanguage();
  else if (clickedButtons.DpadRight)
    selectNextLanguage();
  else if (clickedButtons.Cross)
    selectLanguage();
};

void StateLanguageSelectionScreen::render() {
  t_renderer->renderer2D.render(background);

  if (selectedLanguage != nullptr) {
    t_renderer->renderer2D.render(arrowLeft);
    t_renderer->renderer2D.render(selectedLanguage->icon);
    t_renderer->renderer2D.render(arrowRight);

    FontManager_printText(
        selectedLanguage->title.c_str(),
        FontOptions(Vec2(256, 325), infoColor, 1.0F, TextAlignment::Center));
    FontManager_printText(
        (std::string("Author: ").append(selectedLanguage->author)),
        FontOptions(Vec2(256, 365), infoColor, 0.7F, TextAlignment::Center));

    FontManager_printText(
        (std::string("Revisor: ").append(selectedLanguage->revision)),
        FontOptions(Vec2(256, 380), infoColor, 0.7F, TextAlignment::Center));
  }

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText(selectedLanguage->selectLabel, 35, 407);
};

void StateLanguageSelectionScreen::unloadTextures() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  textureRepo->freeBySprite(background);
  textureRepo->freeBySprite(btnCross);
  textureRepo->freeBySprite(arrowLeft);
  textureRepo->freeBySprite(arrowRight);

  for (size_t i = 0; i < languages.size(); i++) {
    textureRepo->freeBySprite(languages[i].icon);
  }
}

void StateLanguageSelectionScreen::nextState() {
  this->context->setState(new StateMainMenu(this->context));
}

void StateLanguageSelectionScreen::loadLanguagesAvailable() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  std::string pathPrefix = "lang/";
  std::vector<UtilDirectory> languagesFolders =
      Utils::listDir(FileUtils::fromCwd(pathPrefix));
  u8 tempId = 1;

  for (size_t i = 0; i < languagesFolders.size(); i++) {
    const UtilDirectory dir = languagesFolders.at(i);
    if (dir.isDir) {
      std::string languageDir = pathPrefix + dir.name;
      std::string languageInfoPath = languageDir + "/language.json";
      std::ifstream langInfo(FileUtils::fromCwd(languageInfoPath));

      if (langInfo.is_open()) {
        languages.push_back(LanguageInfoModel());
        auto& model = languages.back();

        json data = json::parse(langInfo);

        model.id = tempId++;
        model.path = std::string(languageInfoPath);
        model.code = dir.name;
        model.title = data["info"]["title"].get<std::string>();
        model.author = data["info"]["author"].get<std::string>();
        model.revision = data["info"]["revision"].get<std::string>();
        model.selectLabel =
            data["language_selection_screen"]["select"].get<std::string>();

        model.icon.size.set(FlagIconWidth, FlagIconHeight);
        model.icon.position.set((WIDTH / 2) - FlagIconWidth / 2,
                                (HEIGHT / 2) - FlagIconHeight / 2);
        model.icon.mode = Tyra::SpriteMode::MODE_STRETCH;

        textureRepo->add(FileUtils::fromCwd(languageDir + "/icon.png"))
            ->addLink(model.icon.id);

        langInfo.close();
      }
    }
  }

  if (selectedLanguage == nullptr && languages.size() > 0)
    selectedLanguage = &languages[0];
}

void StateLanguageSelectionScreen::selectLanguage() {
  if (selectedLanguage == nullptr) return;

  std::ifstream langInfo(FileUtils::fromCwd(selectedLanguage->path));
  if (langInfo.is_open()) {
    json language = json::parse(langInfo);
    LanguageManager_SetLanguage(language);

    g_settings.language = selectedLanguage->code;
    SettingsManager::Save();

    langInfo.close();
    return nextState();
  }

  TYRA_ASSERT(false, "Selected language could not be loaded!");
}

void StateLanguageSelectionScreen::selectPreviousLanguage() {
  u16 idx = 0;
  for (size_t i = 0; i < languages.size(); i++)
    if (languages[i].id == selectedLanguage->id) idx = i;

  if (idx == 0)
    selectedLanguage = &languages[languages.size() - 1];
  else
    selectedLanguage = &languages[idx - 1];
}

void StateLanguageSelectionScreen::selectNextLanguage() {
  u16 idx = 0;
  for (size_t i = 0; i < languages.size(); i++)
    if (languages[i].id == selectedLanguage->id) idx = i;

  if (idx == languages.size() - 1)
    selectedLanguage = &languages[0];
  else
    selectedLanguage = &languages[idx + 1];
}