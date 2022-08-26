#include "states/splash_screen/state_splash_screen.hpp"

StateSplashScreen::StateSplashScreen(Context* t_context)
    : GameState(t_context) {
    this->init();
};

StateSplashScreen::~StateSplashScreen() { this->unloadTextures(); };

void StateSplashScreen::init() {
  this->setBgColorBlack(this->context->t_renderer);

  const RendererSettings& rendererSettings =
      this->context->t_renderer->core.getSettings();
  const float width = 512;
  const float height = 512;

  std::string tyracraftSplash = FileUtils::fromCwd("splash/tyracraft.png");
  std::string tyraSplash = FileUtils::fromCwd("splash/tyra.png");

  tyracraft = new Sprite;
  tyracraft->mode = Tyra::MODE_STRETCH;
  tyracraft->size.set(width, height);
  tyracraft->position.set(0, 0);

  tyra = new Sprite;
  tyra->mode = Tyra::MODE_STRETCH;
  tyra->size.set(width, height);
  tyra->position.set(0, 0);

  this->context->t_renderer->core.texture.repository.add(tyracraftSplash)
      ->addLink(tyracraft->id);
  this->context->t_renderer->core.texture.repository.add(tyraSplash)
      ->addLink(tyra->id);
};
void StateSplashScreen::update() {
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
  this->tyra->color.a = alpha;
  this->context->t_renderer->renderer2D.render(tyra);
  if (alpha == 0) hasShowedTyra = 1;
}

void StateSplashScreen::renderTyraCraftSplash() {
  this->tyracraft->color.a = alpha;
  this->context->t_renderer->renderer2D.render(tyracraft);
  if (alpha == 0) hasShowedTyraCraft = 1;
}

void StateSplashScreen::setBgColorBlack(Renderer* renderer) {
  this->context->t_renderer->setClearScreenColor(Color(0.0F, 0.0F, 0.0F));
}

void StateSplashScreen::unloadTextures() {
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteOrMesh(tyracraft->id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteOrMesh(tyra->id)
          ->id);
}

u8 StateSplashScreen::hasFinished() {
  return hasShowedTyraCraft && hasShowedTyra;
}

u8 StateSplashScreen::shouldBeDestroyed() { return this->hasFinished(); }
