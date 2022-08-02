#include "splash_screen.hpp"
#include "file/file_utils.hpp"
#include <string.h>
#include "renderer/models/color.hpp"
#include <debug/debug.hpp>

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Sprite;
using Tyra::TextureRepository;

SplashScreen::SplashScreen(Renderer* t_renderer) {
  this->t_renderer = t_renderer;
  this->setBgColorBlack(t_renderer);

  const RendererSettings& rendererSettings = t_renderer->core.getSettings();
  const float width = 512;
  const float height = 512;

  std::string tyracraftSplash =
      FileUtils::fromCwd("splash/tyracraft.png");
  std::string tyraSplash =
      FileUtils::fromCwd("splash/tyra.png");

  tyracraft = new Sprite;
  tyracraft->setMode(Tyra::MODE_STRETCH);
  tyracraft->size.set(width, height);
  tyracraft->position.set(0, 0);

  tyra = new Sprite;
  tyra->setMode(Tyra::MODE_STRETCH);
  tyra->size.set(width, height);
  tyra->position.set(0, 0);

  t_renderer->core.texture.repository.add(tyracraftSplash)
      ->addLink(tyracraft->getId());
  t_renderer->core.texture.repository.add(tyraSplash)->addLink(tyra->getId());
}

SplashScreen::~SplashScreen() { this->unloadTextures(); }

void SplashScreen::unloadTextures() {
  t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(tyracraft->getId())
          ->getId());
  t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(tyra->getId())
          ->getId());
}

void SplashScreen::render() {
  alpha = isFading ? alpha - 1 : alpha + 1;

  if (alpha == 128) {
    sleep(2);
    isFading = 1;
  } else if (alpha == 0) {
    isFading = 0;
  }

  if (!hasShowedTyra) {
    renderTyraSplash();
    return;
  }

  if (!hasShowedTyraCraft) {
    renderTyraCraftSplash();
    return;
  }
}

void SplashScreen::renderTyraSplash() {
  tyra->color.a = alpha;
  this->t_renderer->renderer2D.render(tyra);
  if (alpha == 0) hasShowedTyra = 1;
}

void SplashScreen::renderTyraCraftSplash() {
  tyracraft->color.a = alpha;
  this->t_renderer->renderer2D.render(tyracraft);
  if (alpha == 0) hasShowedTyraCraft = 1;
}

void SplashScreen::setBgColorBlack(Renderer* renderer) {
  this->t_renderer->setClearScreenColor(Color(0.0F, 0.0F, 0.0F));
}

u8 SplashScreen::hasFinished() { return hasShowedTyraCraft && hasShowedTyra; }

u8 SplashScreen::shouldBeDestroyed() { return this->hasFinished(); }