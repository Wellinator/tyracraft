#include "states/main_menu/screens/screen_about.hpp"
#include "states/main_menu/screens/screen_main.hpp"

ScreenAbout::ScreenAbout(StateMainMenu* t_context) : ScreenBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->init();
}

ScreenAbout::~ScreenAbout() {
  this->t_renderer->getTextureRepository().freeBySprite(about_background);
  this->t_renderer->getTextureRepository().freeBySprite(about_text_1);
  this->t_renderer->getTextureRepository().freeBySprite(about_text_2);
  this->t_renderer->getTextureRepository().freeBySprite(about_text_3);
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
      .add(FileUtils::fromCwd("assets/menu/about_background.png"))
      ->addLink(about_background.id);

  about_text_1.mode = Tyra::MODE_STRETCH;
  about_text_1.size.set(512, 512);
  about_text_1.position.set(0, 0);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/about_text_1.png"))
      ->addLink(about_text_1.id);

  about_text_2.mode = Tyra::MODE_STRETCH;
  about_text_2.size.set(512, 512);
  about_text_2.position.set(0, 0);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/about_text_2.png"))
      ->addLink(about_text_2.id);

  about_text_3.mode = Tyra::MODE_STRETCH;
  about_text_3.size.set(512, 512);
  about_text_3.position.set(0, 0);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/about_text_3.png"))
      ->addLink(about_text_3.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(
      15, this->t_renderer->core.getSettings().getHeight() - 40);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/textures/ui/btn_triangle.png"))
      ->addLink(btnTriangle.id);

  textBack.mode = Tyra::MODE_STRETCH;
  textBack.size.set(64, 15);
  textBack.position.set(15 + 30,
                        this->t_renderer->core.getSettings().getHeight() - 36);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/text_back.png"))
      ->addLink(textBack.id);
}

void ScreenAbout::handleInput() {
  if (this->canReturnToMainMenu()) return this->navigate();
}
void ScreenAbout::navigate() {
  this->context->setScreen(new ScreenMain(this->context));
}

void ScreenAbout::renderAboutText() {
  if (!hasShowedText1) return renderAboutText1();
  if (!hasShowedText2) return renderAboutText2();
  if (!hasShowedText3) return renderAboutText3();
}

void ScreenAbout::renderAboutText1() {
  this->about_text_1.color.a = alpha;
  this->t_renderer->renderer2D.render(&about_text_1);
  if (alpha == 0) hasShowedText1 = 1;
}

void ScreenAbout::renderAboutText2() {
  this->about_text_2.color.a = alpha;
  this->t_renderer->renderer2D.render(&about_text_2);
  if (alpha == 0) hasShowedText2 = 1;
}

void ScreenAbout::renderAboutText3() {
  this->about_text_3.color.a = alpha;
  this->t_renderer->renderer2D.render(&about_text_3);
  if (alpha == 0) hasShowedText3 = 1;
}

u8 ScreenAbout::canReturnToMainMenu() {
  return this->hasShowedText1 && this->hasShowedText2 && this->hasShowedText3;
}
