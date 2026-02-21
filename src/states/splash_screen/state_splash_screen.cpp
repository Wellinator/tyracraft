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
  if (hasFinished()) {
    this->nextState();
    return;
  }

  const auto& clicked = this->context->t_engine->pad.getClicked();
  if (clicked.Start || clicked.Cross) {
    this->nextState();
    return;
  }

  if (wait) {
    timeout += deltaTime;
    if (timeout >= 1.5f) {
      wait = false;
      timeout = 0;
    }
  }

  const double speed = 45 * deltaTime;
  alpha += isFading ? -speed : speed;

  if (alpha > 128) {
    isFading = 1;
    alpha = 128.0f;
    wait = true;
  } else if (alpha < 0) {
    isFading = 0;
    alpha = 0.0f;

    if (!hasShowedTyra) {
      hasShowedTyra = true;
    } else if (!hasShowedTyraCraft) {
      hasShowedTyraCraft = true;
    }
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
}

void StateSplashScreen::renderTyraCraftSplash() {
  this->tyracraft.color.a = alpha;
  this->context->t_engine->renderer.renderer2D.render(tyracraft);
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
