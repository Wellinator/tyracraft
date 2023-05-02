#include "states/main_menu/screens/screen_about.hpp"
#include "states/main_menu/screens/screen_main.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/font/font_options.hpp"

ScreenAbout::ScreenAbout(StateMainMenu* t_context) : ScreenBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  BASE_WIDTH = (t_renderer->core.getSettings().getWidth() / 2) - 10;
  this->init();
}

ScreenAbout::~ScreenAbout() {
  this->t_renderer->getTextureRepository().freeBySprite(about_background);
  this->t_renderer->getTextureRepository().freeBySprite(btnTriangle);
  this->t_renderer->getTextureRepository().freeBySprite(textBack);
}

void ScreenAbout::update() {
  this->handleInput();
  this->alpha = isFading ? alpha - 1 : alpha + 1;
  if (alpha == 128) {
    sleep(5);
    isFading = 1;
  } else if (alpha == 0) {
    isFading = 0;
  }
}

void ScreenAbout::render() {
  this->t_renderer->renderer2D.render(&about_background);
  this->renderAboutText();
  if (this->canReturnToMainMenu()) {
    this->t_renderer->renderer2D.render(&btnTriangle);
    this->t_renderer->renderer2D.render(&textBack);
  }
}

void ScreenAbout::init() {
  about_background.mode = Tyra::MODE_STRETCH;
  about_background.size.set(512, 512);
  about_background.position.set(0, 0);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/menu/about_background.png"))
      ->addLink(about_background.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(
      15, this->t_renderer->core.getSettings().getHeight() - 40);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/btn_triangle.png"))
      ->addLink(btnTriangle.id);

  textBack.mode = Tyra::MODE_STRETCH;
  textBack.size.set(64, 15);
  textBack.position.set(15 + 30,
                        this->t_renderer->core.getSettings().getHeight() - 36);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/menu/text_back.png"))
      ->addLink(textBack.id);
}

void ScreenAbout::handleInput() {
  if (this->canReturnToMainMenu()) return this->navigate();
}
void ScreenAbout::navigate() {
  this->context->setScreen(new ScreenMain(this->context));
}

void ScreenAbout::renderAboutText() {
  if (!hasShowedText1)
    renderAboutText1();
  else if (!hasShowedText2)
    renderAboutText2();
  else if (!hasShowedText3)
    renderAboutText3();
}

void ScreenAbout::renderAboutText1() {
  auto fontOptions = FontOptions();
  const u16 BASE_HEIGHT = 158;
  fontOptions.scale = 0.8;
  fontOptions.alignment = TextAlignment::Center;
  fontOptions.position.x = BASE_WIDTH;
  fontOptions.color.r = 255;
  fontOptions.color.g = 255;
  fontOptions.color.b = 255;
  fontOptions.color.a = alpha;

  fontOptions.position.y = BASE_HEIGHT;
  FontManager_printText("Hey there!", fontOptions);
  fontOptions.position.y += 20;
  FontManager_printText("Wellinator here, I'm gald you came to this screen!",
                        fontOptions);
  fontOptions.position.y += 20;
  fontOptions.position.y += 20;
  FontManager_printText("TyraCraft is an opensource project, powered by",
                        fontOptions);
  fontOptions.position.y += 20;
  FontManager_printText("Tyra Engine, that aims to bring a voxel like game",
                        fontOptions);
  fontOptions.position.y += 20;
  FontManager_printText("to the PlayStation 2 scene.", fontOptions);
  fontOptions.position.y += 20;
  FontManager_printText(
      "We have put a lot of time and effort into this project!", fontOptions);
  fontOptions.position.y += 20;
  FontManager_printText("We hope you enjoy it!", fontOptions);

  if (alpha == 0) hasShowedText1 = 1;
}

void ScreenAbout::renderAboutText2() {
  auto fontOptions = FontOptions();
  const u16 BASE_HEIGHT = 158;
  fontOptions.scale = 0.8;
  fontOptions.alignment = TextAlignment::Center;
  fontOptions.position.x = BASE_WIDTH;
  fontOptions.color.r = 255;
  fontOptions.color.g = 255;
  fontOptions.color.b = 255;
  fontOptions.color.a = alpha;

  fontOptions.position.y = BASE_HEIGHT;
  FontManager_printText("Thank you to all who have contributed in TyraCraft",
                        fontOptions);

  fontOptions.position.y += 20;
  FontManager_printText("You guys are awesome!", fontOptions);

  fontOptions.color.r = 252;
  fontOptions.color.g = 219;
  fontOptions.color.b = 3;

  fontOptions.position.y += 60;
  FontManager_printText("Wellinator, h4570, Wolf3s, LochGames, VORTEX, ",
                        fontOptions);
  fontOptions.position.y += 25;
  FontManager_printText("ApenasNeutrico, Hack, MayconTp, N4RDO, RegenStudio,",
                        fontOptions);
  fontOptions.position.y += 25;
  FontManager_printText("Rocketmanba04, TheTripolino, Vijo96.", fontOptions);

  if (alpha == 0) hasShowedText2 = 1;
}

void ScreenAbout::renderAboutText3() {
  auto fontOptions = FontOptions();
  const u16 BASE_HEIGHT = 158;
  fontOptions.scale = 0.8;
  fontOptions.alignment = TextAlignment::Center;
  fontOptions.position.x = BASE_WIDTH;
  fontOptions.color.r = 255;
  fontOptions.color.g = 255;
  fontOptions.color.b = 255;
  fontOptions.color.a = alpha;

  fontOptions.position.y = BASE_HEIGHT;
  FontManager_printText("Special thanks to all YouTube subscribers",
                        fontOptions);

  fontOptions.position.y += 20;
  FontManager_printText("and those from the Discord Server.", fontOptions);

  fontOptions.color.r = 252;
  fontOptions.color.g = 219;
  fontOptions.color.b = 3;

  fontOptions.position.y += 40;
  FontManager_printText("Join us on Discord!", fontOptions);
  fontOptions.position.y += 20;
  FontManager_printText("https://discord.gg/E2hQAt4Dft", fontOptions);

  fontOptions.position.y += 40;
  FontManager_printText("Follow us!", fontOptions);
  fontOptions.position.y += 20;
  FontManager_printText("https://youtube.com/@TyraCraft", fontOptions);

  if (alpha == 0) hasShowedText3 = 1;
}

u8 ScreenAbout::canReturnToMainMenu() {
  return this->hasShowedText1 && this->hasShowedText2 && this->hasShowedText3;
}
