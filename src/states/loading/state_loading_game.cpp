#include "states/loading/state_loading_game.hpp"
#include "managers/save_manager.hpp"

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

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
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
  } else if (shouldGenerateWorld) {
    return generateWorld();
  } else if (shouldGenerateLight) {
    progressLabel = Label_GeneratingLight;
    return generateLightIncremental();
  } else if (shouldPropagateLiquids) {
    progressLabel = Label_PropagatingLiquids;
    return propagateLiquidsIncremental();
  } else if (shouldGenerateSpawnArea) {
    return generateSpawnArea();
  } else if (shouldLoadSpawnArea) {
    return loadSpawnArea();
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
  stateGamePlay->world->initGeneration();

  setPercent(55.0F);
  shouldInitWorld = 0;
  shouldGenerateWorld = 1;
}

void StateLoadingGame::generateWorld() {
  const bool isDone = stateGamePlay->world->generateStep();

  // Update progress label based on phase
  switch (stateGamePlay->world->currentGenerationPhase) {
    case World::GenerationPhase::Terrain:
      progressLabel = Label_GeneratingTerrain;
      break;
    case World::GenerationPhase::Decoration:
      progressLabel = Label_GeneratingDecoration;
      break;
    case World::GenerationPhase::LightStitch:
    case World::GenerationPhase::Finalize:
      progressLabel = Label_GeneratingLight;
      break;
    default:
      break;
  }

  // Calculate total progress across all 3 phases (55% to 85%)
  float phaseOffset = 0.0f;
  switch (stateGamePlay->world->currentGenerationPhase) {
    case World::GenerationPhase::Decoration:
      phaseOffset = 1.0f;
      break;
    case World::GenerationPhase::LightStitch:
      phaseOffset = 2.0f;
      break;
    case World::GenerationPhase::Finalize:
    case World::GenerationPhase::Complete:
      phaseOffset = 3.0f;
      break;
    default:
      break;
  }

  float phaseProgress =
      (float)stateGamePlay->world->generationRow /
      (float)stateGamePlay->world->generationTotalRows;
  
  // Normalized progress (0.0 to 1.0) across all phases
  float normalizedGenProgress = (phaseOffset + phaseProgress) / 3.0f;
  float totalProgress = 55.0f + (normalizedGenProgress * 30.0f);
  setPercent(totalProgress);

  if (isDone) {
    shouldGenerateWorld = 0;
    shouldGenerateLight = 1;
  }
}

void StateLoadingGame::generateSpawnArea() {
  TYRA_LOG("Generating spawn area...");
  stateGamePlay->world->generateSpawnArea();
  setPercent(92.0F);
  shouldGenerateSpawnArea = 0;
  shouldLoadSpawnArea = 1;
}

void StateLoadingGame::loadSpawnArea() {
  TYRA_LOG("Loading spawn area...");
  stateGamePlay->world->loadSpawnArea();
  setPercent(95.0F);
  shouldLoadSpawnArea = 0;
  shouldInitPlayer = 1;
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

void StateLoadingGame::generateLightIncremental() {
  // Process light generation in phases spread across multiple frames
  switch (lightPhase) {
    case LightGenerationPhase::NotStarted:
      TYRA_LOG("Starting light generation...");
      lightPhase = LightGenerationPhase::InitSunlight;
      setPercent(78.0F);
      break;

    case LightGenerationPhase::InitSunlight:
      TYRA_LOG("Initializing sunlight...");
      stateGamePlay->world->prepareLightModelForLoading();
      stateGamePlay->world->lightPropagation.initSunLight(g_ticksCounter);
      lightPhase = LightGenerationPhase::InitBlockLight;
      setPercent(80.0F);
      break;

    case LightGenerationPhase::InitBlockLight:
      TYRA_LOG("Initializing block light...");
      stateGamePlay->world->lightPropagation.initBlockLight(
          &stateGamePlay->world->blockManager);
      lightPhase = LightGenerationPhase::UpdateSunlight;
      setPercent(81.0F);
      break;

    case LightGenerationPhase::UpdateSunlight:
      // Use budgeted sunlight update (2ms budget ≈ 600k cycles)
      if (stateGamePlay->world->lightPropagation.updateSunlightBudgeted(600000)) {
        lightPhase = LightGenerationPhase::UpdateBlockLight;
        setPercent(83.0F);
      }
      break;

    case LightGenerationPhase::UpdateBlockLight:
      // Use budgeted block light update (2ms budget)
      if (stateGamePlay->world->lightPropagation.updateBlockLightsBudgeted(600000)){
        lightPhase = LightGenerationPhase::ReloadChunks;
        setPercent(85.0F);
      }
      break;

    case LightGenerationPhase::ReloadChunks:
      TYRA_LOG("Reloading light data...");
      stateGamePlay->world->chunkManager.reloadLightDataOfAllChunks();
      lightPhase = LightGenerationPhase::Complete;
      setPercent(87.0F);
      break;

    case LightGenerationPhase::Complete:
      TYRA_LOG("Light generation complete");
      shouldGenerateLight = 0;
      shouldPropagateLiquids = 1;
      lightPhase = LightGenerationPhase::NotStarted;
      break;
  }
}

void StateLoadingGame::propagateLiquidsIncremental() {
  // Process liquid propagation incrementally using budgeted method
  switch (liquidPhase) {
    case LiquidPropagationPhase::NotStarted:
      TYRA_LOG("Starting liquid propagation...");
      liquidPhase = LiquidPropagationPhase::Propagating;
      setPercent(88.0F);
      break;

    case LiquidPropagationPhase::Propagating: {
      // Each frame, process one BFS operation from liquid queues
      // Returns true when queues are drained
      const bool isDone = stateGamePlay->world->liquidPropagation
          .propagateAllBudgeted(16);  // 16ms budget per frame
      if (isDone) {
        TYRA_LOG("Liquid propagation complete");
        liquidPhase = LiquidPropagationPhase::Complete;
      }
      setPercent(isDone ? 97.0F : 90.0F);
      break;
    }

    case LiquidPropagationPhase::Complete:
      TYRA_LOG("Liquid propagation done");
      shouldPropagateLiquids = 0;
      shouldGenerateSpawnArea = 1;
      liquidPhase = LiquidPropagationPhase::NotStarted;
      break;
  }
}

void StateLoadingGame::nextState() {
  TYRA_LOG("nextState");
  stateGamePlay->afterInit();
  SaveManager::SaveGame(stateGamePlay, worldOptions.fullPath.c_str());
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
