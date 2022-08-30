#include "states/loading/state_loading_game.hpp"

StateLoadingGame::StateLoadingGame(Context* t_context) : GameState(t_context) {
  this->stateGamePlay = new StateGamePlay(this->context);
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
  if (this->hasFinished()) {
    this->nextState();
  }

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
  this->_state = LoadingState::Complete;
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

void StateLoadingGame::createEntities() {
  this->stateGamePlay->world = new World();
  this->stateGamePlay->player = new Player(this->context->t_renderer, this->context->t_audio);
  this->stateGamePlay->itemRepository = new ItemRepository();
  this->stateGamePlay->ui = new Ui();
  setPercent(25.0F);
  this->shouldCreatedEntities = 0;
}

void StateLoadingGame::initItemRepository() {
  this->stateGamePlay->itemRepository->init(this->context->t_renderer);

  setPercent(35.0F);
  this->shouldInitItemRepository = 0;
  TYRA_LOG("initItemRepository");
}

void StateLoadingGame::initUI() {
  this->stateGamePlay->ui->init(this->context->t_renderer,
                                this->stateGamePlay->itemRepository,
                                this->stateGamePlay->player);
  setPercent(50.0F);
  this->shouldInitUI = 0;
  TYRA_LOG("initUI");
}

void StateLoadingGame::initWorld() {
  this->stateGamePlay->world->init(this->context->t_renderer,
                                   this->stateGamePlay->itemRepository);
  setPercent(90.0F);
  this->shouldInitWorld = 0;
  TYRA_LOG("initWorld");
}

void StateLoadingGame::initPlayer() {
  this->stateGamePlay->player->mesh->getPosition()->set(
      this->stateGamePlay->world->getGlobalSpawnArea());
  this->stateGamePlay->player->spawnArea.set(
      this->stateGamePlay->world->getLocalSpawnArea());
  setPercent(100.0F);
  this->shouldInitPlayer = 0;
  TYRA_LOG("initPlayer");
}

void StateLoadingGame::nextState() {
  TYRA_LOG("nextState");
  this->context->setState(this->stateGamePlay);
}

void StateLoadingGame::setPercent(float completed) {
  this->_percent = completed;
  this->loadingprogress->size.set(this->_percent / 100 * 253, 9);
}

void StateLoadingGame::setBgColorBlack() {
  this->context->t_renderer->setClearScreenColor(Color(0.0F, 0.0F, 0.0F));
}

bool StateLoadingGame::hasFinished() {
  return this->_state == LoadingState::Complete;
}