#include "states/loading/state_loading_game.hpp"
#include "states/game_play/state_game_play.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"
#include "managers/items_repository.hpp"

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

void StateLoadingGame::update(const float& deltaTime) {
  if (this->hasFinished()) return this->nextState();
  printf("Update state loading...\n");
  this->loadGame();
  printf("Updated state loading...\n");
}

void StateLoadingGame::render() {
  return;
  this->context->t_renderer->renderer2D.render(background);
  this->context->t_renderer->renderer2D.render(loadingSlot);
  this->context->t_renderer->renderer2D.render(loadingprogress);
  this->context->t_renderer->renderer2D.render(loadingStateText);
}

void StateLoadingGame::unload() {
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteId(background->id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteId(loadingSlot->id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteId(loadingprogress->id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteId(loadingStateText->id)
          ->id);
}

void StateLoadingGame::loadGame() {
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  if (this->shouldCreatedEntities) {
    return this->createEntities();
  } else if (this->shouldInitItemRepository) {
    return this->initItemRepository();
  } else if (this->shouldInitUI) {
    return this->initUI();
  } else if (this->shouldInitWorld) {
    return this->initWorld();
  } else if (this->shouldInitPlayer) {
    return this->initPlayer();
  }
  setState(LoadingState::Complete);
  printf("END\n");
}

void StateLoadingGame::createEntities() {
  printf("createEntities\n");
  this->context->world = new World();
  this->context->player =
      new Player(this->context->t_renderer, this->context->t_audio);
  this->context->itemRepository = new ItemRepository();
  this->context->ui = new Ui();
  setPercent(25.0F);
  this->shouldCreatedEntities = 0;
}

void StateLoadingGame::initItemRepository() {
  printf("initItemRepository\n");
  this->context->itemRepository->init(this->context->t_renderer);
  setPercent(35.0F);
  this->shouldInitItemRepository = 0;
}

void StateLoadingGame::initUI() {
  printf("initUI\n");
  this->context->ui->init(this->context->t_renderer,
                          this->context->itemRepository, this->context->player);
  setPercent(50.0F);
  this->shouldInitUI = 0;
}

void StateLoadingGame::initWorld() {
  printf("initWorld\n");
  this->context->world->init(this->context->t_renderer,
                             this->context->itemRepository);
  setPercent(90.0F);
  this->shouldInitWorld = 0;
}

void StateLoadingGame::initPlayer() {
  printf("initPlayer\n");
  this->context->player->mesh->getPosition()->set(
      this->context->world->getGlobalSpawnArea());
  this->context->player->spawnArea.set(
      this->context->world->getLocalSpawnArea());
  setPercent(100.0F);
  this->shouldInitPlayer = 0;
}

void StateLoadingGame::nextState() {
  this->context->setState(new StateGamePlay(this->context));
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
