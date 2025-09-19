#include "states/loading/state_loading_game.hpp"

StateLoadingGame::StateLoadingGame(Context* t_context,
                                   const NewGameOptions& options)
    : GameState(t_context) {
  worldOptions = options;
  stateGamePlay = new StateGamePlay(context, options.gameMode);
  init();
}

StateLoadingGame::~StateLoadingGame() { unload(); }

void StateLoadingGame::init() {
  setBgColorBlack();
  progressLabel = Label_Loading;

  const RendererSettings& rendererSettings =
      context->t_engine->renderer.core.getSettings();
  const float width = rendererSettings.getWidth();
  const float height = rendererSettings.getHeight();
  BASE_HEIGHT = height - 120;

  // Background
  std::string backgroundTex =
      FileUtils::fromCwd("textures/gui/loading/background.png");
  background = new Sprite;
  background->mode = Tyra::MODE_STRETCH;
  background->size.set(512, 512);
  background->position.set(0, 0);
  context->t_engine->renderer.core.texture.repository.add(backgroundTex)
      ->addLink(background->id);

  // Loading slot
  std::string loadingSlotTex =
      FileUtils::fromCwd("textures/gui/loading/empty_loading_bar.png");
  loadingSlot = new Sprite;
  loadingSlot->mode = Tyra::MODE_STRETCH;
  loadingSlot->size.set(256, 16);
  loadingSlot->position.set(width / 2 - 128, BASE_HEIGHT + 25);
  context->t_engine->renderer.core.texture.repository.add(loadingSlotTex)
      ->addLink(loadingSlot->id);

  // Loading bar
  std::string loadingprogressTex =
      FileUtils::fromCwd("textures/gui/loading/load.png");
  loadingprogress = new Sprite;
  loadingprogress->mode = Tyra::MODE_STRETCH;
  loadingprogress->size.set(_percent / 100 * 253, 9);
  loadingprogress->position.set(width / 2 - 125, BASE_HEIGHT + 28);
  context->t_engine->renderer.core.texture.repository.add(loadingprogressTex)
      ->addLink(loadingprogress->id);
}

void StateLoadingGame::update(const float& deltaTime) {
  if (hasFinished()) {
    nextState();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  if (shouldCreatedEntities) {
    progressLabel = Label_CreatingEntities;
    return createEntities();
  } else if (shouldInitItemRepository) {
    progressLabel = Label_LoadingItemsRepo;
    return initItemRepository();
  } else if (shouldInitUI) {
    progressLabel = Label_LoadingUI;
    return initUI();
  } else if (shouldInitWorld) {
    progressLabel = Label_LoadingWorld;
    return initWorld();
  } else if (shouldInitPlayer) {
    progressLabel = Label_LoadingPlayer;
    return initPlayer();
  }
  _state = LoadingState::Complete;
}

void StateLoadingGame::render() {
  context->t_engine->renderer.renderer2D.render(background);
  context->t_engine->renderer.renderer2D.render(loadingSlot);
  context->t_engine->renderer.renderer2D.render(loadingprogress);

  FontManager::getInstance()->printText(progressLabel, progressLabelOptions);
}

void StateLoadingGame::unload() {
  context->t_engine->renderer.getTextureRepository().freeBySprite(*background);
  context->t_engine->renderer.getTextureRepository().freeBySprite(*loadingSlot);
  context->t_engine->renderer.getTextureRepository().freeBySprite(
      *loadingprogress);

  delete background;
  delete loadingSlot;
  delete loadingprogress;
}

void StateLoadingGame::createEntities() {
  stateGamePlay->plevel = new Level(worldOptions.seed);

  stateGamePlay->world = new World(worldOptions, stateGamePlay->plevel);
  stateGamePlay->itemRepository = new ItemRepository();

  stateGamePlay->player =
      new Player(stateGamePlay->plevel, &context->t_engine->renderer,
                 stateGamePlay->itemRepository,
                 stateGamePlay->world->getWorldLightModel());

  stateGamePlay->ui = new Ui();
  setPercent(25.0F);
  shouldCreatedEntities = 0;
}

void StateLoadingGame::initItemRepository() {
  stateGamePlay->itemRepository->init(&context->t_engine->renderer,
                                      worldOptions.texturePack);

  setPercent(35.0F);
  shouldInitItemRepository = 0;
  TYRA_LOG("initItemRepository");
}

void StateLoadingGame::initUI() {
  stateGamePlay->ui->init(&context->t_engine->renderer,
                          stateGamePlay->itemRepository, stateGamePlay->player);
  setPercent(50.0F);
  shouldInitUI = 0;
  TYRA_LOG("initUI");
}

void StateLoadingGame::initWorld() {
  stateGamePlay->world->init(&context->t_engine->renderer,
                             stateGamePlay->itemRepository);
  stateGamePlay->world->generate();
  stateGamePlay->world->generateLight();
  stateGamePlay->world->propagateLiquids();

  TYRA_LOG("Generating spawn area...");
  stateGamePlay->world->generateSpawnArea();

  TYRA_LOG("Loading spawn area...");
  stateGamePlay->world->loadSpawnArea();

  setPercent(90.0F);
  shouldInitWorld = 0;
  TYRA_LOG("initWorld");
}

void StateLoadingGame::initPlayer() {
  TYRA_LOG("Initiating player...");

  TYRA_LOG("Setting player position...");
  stateGamePlay->player->setPosition(
      stateGamePlay->world->getGlobalSpawnArea());

  TYRA_LOG("Setting player spawn area...");
  stateGamePlay->player->spawnArea.set(
      stateGamePlay->world->getLocalSpawnArea());
  stateGamePlay->context->t_camera->setFirstPerson();

  setPercent(100.0F);

  shouldInitPlayer = 0;

  TYRA_LOG("Player initiated!");
}

void StateLoadingGame::nextState() {
  TYRA_LOG("nextState");
  stateGamePlay->afterInit();
  context->setState(stateGamePlay);
}

void StateLoadingGame::setPercent(float completed) {
  _percent = completed;
  loadingprogress->size.set(_percent / 100 * 250, 9);
}

void StateLoadingGame::setBgColorBlack() {
  context->t_engine->renderer.setClearScreenColor(Color(0.0F, 0.0F, 0.0F));
}

bool StateLoadingGame::hasFinished() {
  return _state == LoadingState::Complete;
}
