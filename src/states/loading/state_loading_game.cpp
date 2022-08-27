#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Vec4;

StateLoadingGame::StateLoadingGame(Context* t_context) : GameState(t_context) {
  this->init();
}

StateLoadingGame::~StateLoadingGame() { this->unload(); }

void StateLoadingGame::init() {
  this->setBgColorBlack();

  const RendererSettings& rendererSettings =
      this->context->t_renderer->core.getSettings();
  const float width = rendererSettings.getWidth();
  const float height = rendererSettings.getHeight();
  this->BASE_HEIGHT = height - 120;

  // Background
  std::string backgroundTex = FileUtils::fromCwd("loading/background.png");
  background = new Sprite;
  background->mode = Tyra::MODE_STRETCH;
  background->size.set(512, 512);
  background->position.set(0, 0);
  this->context->t_renderer->core.texture.repository.add(backgroundTex)
      ->addLink(background->id);

  // State desc
  std::string stateLoadingText = FileUtils::fromCwd("loading/loading.png");
  loadingStateText = new Sprite;
  loadingStateText->mode = Tyra::MODE_STRETCH;
  loadingStateText->size.set(256, 16);
  loadingStateText->position.set(width / 2 - 128, BASE_HEIGHT);
  this->context->t_renderer->core.texture.repository.add(stateLoadingText)
      ->addLink(loadingStateText->id);

  // Loading slot
  std::string loadingSlotTex =
      FileUtils::fromCwd("loading/empty_loading_bar.png");
  loadingSlot = new Sprite;
  loadingSlot->mode = Tyra::MODE_STRETCH;
  loadingSlot->size.set(256, 16);
  loadingSlot->position.set(width / 2 - 128, BASE_HEIGHT + 25);
  this->context->t_renderer->core.texture.repository.add(loadingSlotTex)
      ->addLink(loadingSlot->id);

  // Loading bar
  std::string loadingprogressTex = FileUtils::fromCwd("loading/load.png");
  loadingprogress = new Sprite;
  loadingprogress->mode = Tyra::MODE_STRETCH;
  loadingprogress->size.set(this->_percent / 100 * 253, 9);
  loadingprogress->position.set(width / 2 - 125, BASE_HEIGHT + 28);
  this->context->t_renderer->core.texture.repository.add(loadingprogressTex)
      ->addLink(loadingprogress->id);
}

void StateLoadingGame::update() {
  if (this->hasFinished()) return this->nextState();
  this->loadGame();
}

void StateLoadingGame::render() {
  this->context->t_renderer->renderer2D.render(background);
  this->context->t_renderer->renderer2D.render(loadingSlot);
  this->context->t_renderer->renderer2D.render(loadingprogress);
  this->context->t_renderer->renderer2D.render(loadingStateText);
}

void StateLoadingGame::unload() {
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteOrMesh(background->id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteOrMesh(loadingSlot->id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteOrMesh(loadingprogress->id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteOrMesh(loadingStateText->id)
          ->id);
}

void StateLoadingGame::loadGame() {
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  if (this->shouldCreatedEntities) {
    this->createEntities();
    return;
  } else if (this->shouldInitItemRepository) {
    this->initItemRepository();
    return;
  } else if (this->shouldInitUI) {
    this->initUI();
    return;
  } else if (this->shouldInitWorld) {
    this->initWorld();
    return;
  } else if (this->shouldInitPlayer) {
    this->initPlayer();
    return;
  }
}

void StateLoadingGame::createEntities() {
  this->context->world = new World();
  this->context->player = new Player(this->context->t_renderer, this->context->t_audio);
  this->context->itemRepository = new ItemRepository();
  this->context->ui = new Ui();
  setPercent(25.0F);
  this->shouldCreatedEntities = 0;
}

void StateLoadingGame::initItemRepository() {
  this->context->itemRepository->init(this->context->t_renderer);
  setPercent(35.0F);
  this->shouldInitItemRepository = 0;
}

void StateLoadingGame::initUI() {
  this->context->ui->init(this->context->t_renderer, this->context->itemRepository, this->context->player);
  setPercent(50.0F);
  this->shouldInitUI = 0;
}

void StateLoadingGame::initWorld() {
  this->context->world->init(this->context->t_renderer, this->context->itemRepository);
  setPercent(90.0F);
  this->shouldInitWorld = 0;
}

void StateLoadingGame::initPlayer() {
  this->context->player->mesh->getPosition()->set(this->context->world->getGlobalSpawnArea());
  this->context->player->spawnArea.set(this->context->world->getLocalSpawnArea());
  setPercent(100.0F);
  setState(LoadingState::Complete);
  this->shouldInitPlayer = 0;
}

void StateLoadingGame::nextState() {
  // TODO: move to in game state;
  // this->context->setState(new StateMainMenu(this->context));
}

void StateLoadingGame::setState(LoadingState state) { this->_state = state; }

void StateLoadingGame::setPercent(float completed) {
  this->_percent = completed;
  this->loadingprogress->size.set(this->_percent / 100 * 253, 9);
}

void StateLoadingGame::setBgColorBlack() {
  this->context->t_renderer->setClearScreenColor(Color(0.0F, 0.0F, 0.0F));
}

u8 StateLoadingGame::hasFinished() {
  return this->_state == LoadingState::Complete;
}
