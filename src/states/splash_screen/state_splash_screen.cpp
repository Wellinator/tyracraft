#include "states/splash_screen/state_splash_screen.hpp"
#include "states/language_selection/language_loader_screen.hpp"

StateSplashScreen::StateSplashScreen(Context* t_context)
    : GameState(t_context) {
  this->init();
};

StateSplashScreen::~StateSplashScreen() { this->unloadTextures(); };

void StateSplashScreen::init() {
  this->setBgColorBlack(&this->context->t_engine->renderer);

  const float width = 512;
  const float height = 512;

  std::string tyracraftSplash =
      FileUtils::fromCwd("textures/gui/splash/tyracraft.png");
  std::string tyraSplash = FileUtils::fromCwd("textures/gui/splash/tyra.png");

  tyracraft.mode = Tyra::MODE_STRETCH;
  tyracraft.size.set(width, height);
  tyracraft.position.set(0, 0);

  tyra.mode = Tyra::MODE_STRETCH;
  tyra.size.set(width, height);
  tyra.position.set(0, 0);

  this->context->t_engine->renderer.core.texture.repository
      .add(tyracraftSplash)
      ->addLink(tyracraft.id);
  this->context->t_engine->renderer.core.texture.repository.add(tyraSplash)
      ->addLink(tyra.id);
};

void StateSplashScreen::update(const float& deltaTime) {
  if (hasFinished()) this->nextState();
  this->alpha = isFading ? alpha - 1 : alpha + 1;

  if (alpha == 128) {
    sleep(2);
    isFading = 1;
  } else if (alpha == 0) {
    isFading = 0;
  }
};

void StateSplashScreen::render() {
  if (!hasShowedTyra) {
    renderTyraSplash();
    return;
  }

  if (!hasShowedTyraCraft) {
    renderTyraCraftSplash();
    return;
  }
};

void StateSplashScreen::renderTyraSplash() {
  this->tyra.color.a = alpha;
  this->context->t_engine->renderer.renderer2D.render(tyra);
  if (alpha == 0) hasShowedTyra = 1;
}

void StateSplashScreen::renderTyraCraftSplash() {
  this->tyracraft.color.a = alpha;
  this->context->t_engine->renderer.renderer2D.render(tyracraft);
  if (alpha == 0) hasShowedTyraCraft = 1;
}

void StateSplashScreen::setBgColorBlack(Renderer* renderer) {
  this->context->t_engine->renderer.setClearScreenColor(
      Color(0.0F, 0.0F, 0.0F));
}

void StateSplashScreen::unloadTextures() {
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(
      tyracraft);
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(tyra);
}

u8 StateSplashScreen::hasFinished() {
  return hasShowedTyraCraft && hasShowedTyra;
}

void StateSplashScreen::nextState() {
  this->context->setState(new StateLanguageLoaderScreen(this->context));
}
