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
  const float width = rendererSettings.getWidth();
  const float height = rendererSettings.getHeight();
  const float tileWidth = width / 5.0F;
  const float tileHeight = height / 3.5F;
  const float heightOffset = 32.0F;

  std::string tyracraftSplashBasePath =
      FileUtils::fromCwd("assets/splash_screen/tyracraft/");
  std::string tyraSplashBasePath =
      FileUtils::fromCwd("assets/splash_screen/tyra/");

  u8 index = 0;
  for (u8 row = 0; row < ROWS; row++) {
    for (u8 col = 0; col < COLS; col++) {
      
      std::string image_index = std::to_string(index + 1) + std::string(".png");

      tyracraft_grid[index] = new Sprite;
      tyracraft_grid[index]->setMode(Tyra::MODE_STRETCH);
      tyracraft_grid[index]->size.set(tileWidth, tileHeight);
      tyracraft_grid[index]->position.set(
          (col * tileWidth) + col, (row * tileHeight) + row + heightOffset);

      tyra_grid[index] = new Sprite;
      tyra_grid[index]->setMode(Tyra::MODE_STRETCH);
      tyra_grid[index]->size.set(tileWidth, tileHeight);
      tyra_grid[index]->position.set((col * tileWidth) + col,
                                     (row * tileHeight) + row + heightOffset);

      t_renderer->core.texture.repository
          .add(tyracraftSplashBasePath + image_index)
          ->addLink(tyracraft_grid[index]->getId());
      t_renderer->core.texture.repository.add(tyraSplashBasePath + image_index)
          ->addLink(tyra_grid[index]->getId());
      index++;
    }
  }
}

SplashScreen::~SplashScreen() { this->unloadTextures(); }

void SplashScreen::unloadTextures() {
  for (u8 index = 0; index < ROWS * COLS; index++) {
    t_renderer->core.texture.repository.free(
        t_renderer->core.texture.repository
            .getBySpriteOrMesh(tyracraft_grid[index]->getId())
            ->getId());
    t_renderer->core.texture.repository.free(
        t_renderer->core.texture.repository
            .getBySpriteOrMesh(tyra_grid[index]->getId())
            ->getId());
  }
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
  for (u8 index = 0; index < ROWS * COLS; index++) {
    tyra_grid[index]->color.a = alpha;
    this->t_renderer->renderer2D.render(tyra_grid[index]);
    if (alpha == 0) hasShowedTyra = 1;
  }
}

void SplashScreen::renderTyraCraftSplash() {
  for (u8 index = 0; index < ROWS * COLS; index++) {
    tyracraft_grid[index]->color.a = alpha;
    this->t_renderer->renderer2D.render(tyracraft_grid[index]);
    if (alpha == 0) hasShowedTyraCraft = 1;
  }
}

void SplashScreen::setBgColorBlack(Renderer* renderer) {
  this->t_renderer->setClearScreenColor(Color(0.0F, 0.0F, 0.0F));
}

u8 SplashScreen::hasFinished() { return hasShowedTyraCraft && hasShowedTyra; }

u8 SplashScreen::shouldBeDestroyed() { return this->hasFinished(); }