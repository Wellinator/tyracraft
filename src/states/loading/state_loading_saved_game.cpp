#include "states/loading/state_loading_saved_game.hpp"
#include "managers/save_manager.hpp"

StateLoadingSavedGame::StateLoadingSavedGame(
    Context* t_context, const std::string save_file_full_path)
    : GameState(t_context), saveFileFullPath(save_file_full_path) {
  worldOptions =
      SaveManager::GetNewGameOptionsFromSaveFile(save_file_full_path.c_str());
  stateGamePlay = new StateGamePlay(context, GameMode::Creative);
  init();
}

StateLoadingSavedGame::~StateLoadingSavedGame() { unload(); }

void StateLoadingSavedGame::init() {
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

void StateLoadingSavedGame::update(const float& deltaTime) {
  if (hasFinished()) {
    nextState();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

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
  } else if (shouldLoadSavedData) {
    progressLabel = Label_LoadingSave;
    return loadSavedData();
  } else if (shouldInitPlayer) {
    progressLabel = Label_LoadingPlayer;
    return initPlayer();
  }
  _state = LoadingState::Complete;
}

void StateLoadingSavedGame::render() {
  context->t_engine->renderer.renderer2D.render(background);
  context->t_engine->renderer.renderer2D.render(loadingSlot);
  context->t_engine->renderer.renderer2D.render(loadingprogress);

  FontManager::getInstance()->printText(progressLabel, progressLabelOptions);
}

void StateLoadingSavedGame::unload() {
  context->t_engine->renderer.getTextureRepository().freeBySprite(*background);
  context->t_engine->renderer.getTextureRepository().freeBySprite(*loadingSlot);
  context->t_engine->renderer.getTextureRepository().freeBySprite(
      *loadingprogress);

  delete background;
  delete loadingSlot;
  delete loadingprogress;
  delete worldOptions;
}

void StateLoadingSavedGame::createEntities() {
  stateGamePlay->plevel = new Level(worldOptions->seed);
  stateGamePlay->world = new World(*worldOptions, stateGamePlay->plevel);
  stateGamePlay->itemRepository = new ItemRepository();

  stateGamePlay->player =
      new Player(stateGamePlay->plevel, &context->t_engine->renderer,
                 stateGamePlay->itemRepository,
                 stateGamePlay->world->getWorldLightModel());
  stateGamePlay->ui = new Ui();
  setPercent(25.0F);
  shouldCreatedEntities = 0;
}

void StateLoadingSavedGame::initItemRepository() {
  stateGamePlay->itemRepository->init(&context->t_engine->renderer,
                                      worldOptions->texturePack);

  setPercent(35.0F);
  shouldInitItemRepository = 0;
  TYRA_LOG("initItemRepository");
}

void StateLoadingSavedGame::initUI() {
  stateGamePlay->ui->init(&context->t_engine->renderer,
                          stateGamePlay->itemRepository, stateGamePlay->player);
  setPercent(50.0F);
  shouldInitUI = 0;
  TYRA_LOG("initUI");
}

void StateLoadingSavedGame::initWorld() {
  stateGamePlay->world->init(&context->t_engine->renderer,
                             stateGamePlay->itemRepository);
  setPercent(70.0F);
  shouldInitWorld = 0;
  TYRA_LOG("initWorld");
}

void StateLoadingSavedGame::loadSavedData() {
  SaveManager::LoadSavedGame(stateGamePlay, saveFileFullPath.c_str());
  stateGamePlay->world->generateLight();
  
  // Process liquid propagation after light generation (same as new world)
  stateGamePlay->world->propagateLiquids();
  
  stateGamePlay->world->loadSpawnArea();

  setPercent(80.0F);
  shouldLoadSavedData = 0;
  TYRA_LOG("loadSavedWorld");
}

void StateLoadingSavedGame::initPlayer() {
  TYRA_LOG("Initializing player...");
  stateGamePlay->player->position.set(
      stateGamePlay->world->getGlobalSpawnArea());
  stateGamePlay->player->spawnArea.set(
      stateGamePlay->world->getLocalSpawnArea());
  stateGamePlay->context->t_camera->setFirstPerson();
  
  // Force immediate water state check (avoid 5-tick delay)
  stateGamePlay->player->updateStateInWater();
  
  setPercent(100.0F);
  shouldInitPlayer = 0;
  TYRA_LOG("Player initialized");
}

void StateLoadingSavedGame::nextState() {
  TYRA_LOG("nextState");
  stateGamePlay->afterInit();
  context->setState(stateGamePlay);
}

void StateLoadingSavedGame::setPercent(float completed) {
  _percent = completed;
  loadingprogress->size.set(_percent / 100 * 250, 9);
}

void StateLoadingSavedGame::setBgColorBlack() {
  context->t_engine->renderer.setClearScreenColor(Color(0.0F, 0.0F, 0.0F));
}

bool StateLoadingSavedGame::hasFinished() {
  return _state == LoadingState::Complete;
}
