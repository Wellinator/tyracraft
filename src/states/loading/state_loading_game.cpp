#include "states/loading/state_loading_game.hpp"

StateLoadingGame::StateLoadingGame(Context* t_context,
                                   const NewGameOptions& options)
    : GameState(t_context) {
  this->worldOptions = options;
  this->stateGamePlay = new StateGamePlay(this->context, options.gameMode);
  this->init();
}

StateLoadingGame::~StateLoadingGame() { this->unload(); }

void StateLoadingGame::init() {
  this->setBgColorBlack();
  progressLabel = Label_Loading;

  const RendererSettings& rendererSettings =
      this->context->t_engine->renderer.core.getSettings();
  const float width = rendererSettings.getWidth();
  const float height = rendererSettings.getHeight();
  this->BASE_HEIGHT = height - 120;

  // Background
  std::string backgroundTex =
      FileUtils::fromCwd("textures/gui/loading/background.png");
  background = new Sprite;
  background->mode = Tyra::MODE_STRETCH;
  background->size.set(512, 512);
  background->position.set(0, 0);
  this->context->t_engine->renderer.core.texture.repository.add(backgroundTex)
      ->addLink(background->id);

  // Loading slot
  std::string loadingSlotTex =
      FileUtils::fromCwd("textures/gui/loading/empty_loading_bar.png");
  loadingSlot = new Sprite;
  loadingSlot->mode = Tyra::MODE_STRETCH;
  loadingSlot->size.set(256, 16);
  loadingSlot->position.set(width / 2 - 128, BASE_HEIGHT + 25);
  this->context->t_engine->renderer.core.texture.repository.add(loadingSlotTex)
      ->addLink(loadingSlot->id);

  // Loading bar
  std::string loadingprogressTex =
      FileUtils::fromCwd("textures/gui/loading/load.png");
  loadingprogress = new Sprite;
  loadingprogress->mode = Tyra::MODE_STRETCH;
  loadingprogress->size.set(this->_percent / 100 * 253, 9);
  loadingprogress->position.set(width / 2 - 125, BASE_HEIGHT + 28);
  this->context->t_engine->renderer.core.texture.repository
      .add(loadingprogressTex)
      ->addLink(loadingprogress->id);
}

void StateLoadingGame::update(const float& deltaTime) {
  if (this->hasFinished()) {
    this->nextState();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  if (this->shouldCreatedEntities) {
    progressLabel = Label_CreatingEntities;
    return this->createEntities();
  } else if (this->shouldInitItemRepository) {
    progressLabel = Label_LoadingItemsRepo;
    return this->initItemRepository();
  } else if (this->shouldInitUI) {
    progressLabel = Label_LoadingUI;
    return this->initUI();
  } else if (this->shouldInitWorld) {
    progressLabel = Label_LoadingWorld;
    return this->initWorld();
  } else if (this->shouldInitPlayer) {
    progressLabel = Label_LoadingPlayer;
    return this->initPlayer();
  }
  this->_state = LoadingState::Complete;
}

void StateLoadingGame::render() {
  this->context->t_engine->renderer.renderer2D.render(background);
  this->context->t_engine->renderer.renderer2D.render(loadingSlot);
  this->context->t_engine->renderer.renderer2D.render(loadingprogress);

  FontManager_printText(progressLabel, progressLabelOptions);
}

void StateLoadingGame::unload() {
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(
      *background);
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(
      *loadingSlot);
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(
      *loadingprogress);

  delete background;
  delete loadingSlot;
  delete loadingprogress;
}

void StateLoadingGame::createEntities() {
  this->stateGamePlay->world = new World(this->worldOptions);
  this->stateGamePlay->itemRepository = new ItemRepository();

  this->stateGamePlay->player = new Player(
      &this->context->t_engine->renderer, this->context->t_soundManager,
      &this->stateGamePlay->world->blockManager,
      this->stateGamePlay->itemRepository,
      this->stateGamePlay->world->getWorldLightModel());

  this->stateGamePlay->ui = new Ui();
  setPercent(25.0F);
  this->shouldCreatedEntities = 0;
}

void StateLoadingGame::initItemRepository() {
  this->stateGamePlay->itemRepository->init(&this->context->t_engine->renderer,
                                            this->worldOptions.texturePack);

  setPercent(35.0F);
  this->shouldInitItemRepository = 0;
  TYRA_LOG("initItemRepository");
}

void StateLoadingGame::initUI() {
  this->stateGamePlay->ui->init(&this->context->t_engine->renderer,
                                this->stateGamePlay->itemRepository,
                                this->stateGamePlay->player);
  setPercent(50.0F);
  this->shouldInitUI = 0;
  TYRA_LOG("initUI");
}

void StateLoadingGame::initWorld() {
  this->stateGamePlay->world->init(&this->context->t_engine->renderer,
                                   this->stateGamePlay->itemRepository,
                                   this->context->t_soundManager);
  this->stateGamePlay->world->generate();
  this->stateGamePlay->world->generateLight();
  this->stateGamePlay->world->propagateLiquids();

  TYRA_LOG("Generating spawn area...");
  this->stateGamePlay->world->generateSpawnArea();

  TYRA_LOG("Loading spawn area...");
  this->stateGamePlay->world->loadSpawnArea();

  setPercent(90.0F);
  this->shouldInitWorld = 0;
  TYRA_LOG("initWorld");
}

void StateLoadingGame::initPlayer() {
  TYRA_LOG("Initiating player...");

  TYRA_LOG("Setting player position...");
  this->stateGamePlay->player->mesh->getPosition()->set(
      this->stateGamePlay->world->getGlobalSpawnArea());

  TYRA_LOG("Setting player spawn area...");
  this->stateGamePlay->player->spawnArea.set(
      this->stateGamePlay->world->getLocalSpawnArea());
  this->stateGamePlay->context->t_camera->setFirstPerson();

  setPercent(100.0F);

  this->shouldInitPlayer = 0;

  TYRA_LOG("Player initiated!");
}

void StateLoadingGame::nextState() {
  TYRA_LOG("nextState");
  this->stateGamePlay->afterInit();
  this->context->setState(this->stateGamePlay);
}

void StateLoadingGame::setPercent(float completed) {
  this->_percent = completed;
  this->loadingprogress->size.set(this->_percent / 100 * 250, 9);
}

void StateLoadingGame::setBgColorBlack() {
  this->context->t_engine->renderer.setClearScreenColor(
      Color(0.0F, 0.0F, 0.0F));
}

bool StateLoadingGame::hasFinished() {
  return this->_state == LoadingState::Complete;
}
