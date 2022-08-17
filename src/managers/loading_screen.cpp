#include "managers/loading_screen.hpp"
#include "file/file_utils.hpp"
#include "renderer/models/color.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Vec4;

LoadingScreen::LoadingScreen() {}

LoadingScreen::~LoadingScreen() { this->unload(); }

void LoadingScreen::init(Renderer* t_renderer) {
  this->t_renderer = t_renderer;
  this->setBgColorBlack(t_renderer);

  const RendererSettings& rendererSettings = t_renderer->core.getSettings();
  const float width = rendererSettings.getWidth();
  const float height = rendererSettings.getHeight();
  this->BASE_HEIGHT = height - 120;

  // Background
  std::string backgroundTex = FileUtils::fromCwd("loading/background.png");
  background = new Sprite;
  background->setMode(Tyra::MODE_STRETCH);
  background->size.set(512, 512);
  background->position.set(0, 0);
  t_renderer->core.texture.repository.add(backgroundTex)
      ->addLink(background->id);

  // State desc
  std::string stateLoadingText = FileUtils::fromCwd("loading/loading.png");
  loadingStateText = new Sprite;
  loadingStateText->setMode(Tyra::MODE_STRETCH);
  loadingStateText->size.set(256, 16);
  loadingStateText->position.set(width / 2 - 128, BASE_HEIGHT);
  t_renderer->core.texture.repository.add(stateLoadingText)
      ->addLink(loadingStateText->id);

  // Loading slot
  std::string loadingSlotTex =
      FileUtils::fromCwd("loading/empty_loading_bar.png");
  loadingSlot = new Sprite;
  loadingSlot->setMode(Tyra::MODE_STRETCH);
  loadingSlot->size.set(256, 16);
  loadingSlot->position.set(width / 2 - 128, BASE_HEIGHT + 25);
  t_renderer->core.texture.repository.add(loadingSlotTex)
      ->addLink(loadingSlot->id);

  // Loading bar
  std::string loadingprogressTex = FileUtils::fromCwd("loading/load.png");
  loadingprogress = new Sprite;
  loadingprogress->setMode(Tyra::MODE_STRETCH);
  loadingprogress->size.set(this->_percent / 100 * 253, 9);
  loadingprogress->position.set(width / 2 - 125, BASE_HEIGHT + 28);
  t_renderer->core.texture.repository.add(loadingprogressTex)
      ->addLink(loadingprogress->id);
}

void LoadingScreen::setState(LoadingState state) { this->_state = state; }

void LoadingScreen::setPercent(float completed) {
  this->_percent = completed;
  this->loadingprogress->size.set(this->_percent / 100 * 253, 9);
}

void LoadingScreen::setBgColorBlack(Renderer* renderer) {
  this->t_renderer->setClearScreenColor(Color(0.0F, 0.0F, 0.0F));
}

u8 LoadingScreen::hasFinished() {
  return this->_state == LoadingState::Complete;
}

u8 LoadingScreen::shouldBeDestroyed() { return this->hasFinished(); }

void LoadingScreen::render() {
  this->t_renderer->renderer2D.render(background);
  this->t_renderer->renderer2D.render(loadingSlot);
  this->t_renderer->renderer2D.render(loadingprogress);
  this->t_renderer->renderer2D.render(loadingStateText);
}

void LoadingScreen::unload() {
  this->t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(background->id)
          ->id);
  this->t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(loadingSlot->id)
          ->id);
  this->t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository
          .getBySpriteOrMesh(loadingprogress->id)
          ->id);
  this->t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository
          .getBySpriteOrMesh(loadingStateText->id)
          ->id);
}
