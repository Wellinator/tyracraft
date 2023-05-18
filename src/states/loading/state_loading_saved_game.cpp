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

StateLoadingSavedGame::~StateLoadingSavedGame() { this->unload(); }

void StateLoadingSavedGame::init() {
  this->setBgColorBlack();

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

  // State desc
  std::string stateLoadingText =
      FileUtils::fromCwd("textures/gui/loading/loading.png");
  loadingStateText = new Sprite;
  loadingStateText->mode = Tyra::MODE_STRETCH;
  loadingStateText->size.set(256, 16);
  loadingStateText->position.set(width / 2 - 128, BASE_HEIGHT);
  this->context->t_engine->renderer.core.texture.repository
      .add(stateLoadingText)
      ->addLink(loadingStateText->id);

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

void StateLoadingSavedGame::update(const float& deltaTime) {
  if (this->hasFinished()) {
    this->nextState();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  if (this->shouldCreatedEntities) {
    return this->createEntities();
  } else if (this->shouldInitItemRepository) {
    return this->initItemRepository();
  } else if (this->shouldInitUI) {
    return this->initUI();
  } else if (this->shouldInitWorld) {
    return this->initWorld();
  } else if (this->shouldLoadSavedData) {
    return this->loadSavedData();
  } else if (this->shouldInitPlayer) {
    return this->initPlayer();
  }
  this->_state = LoadingState::Complete;
}

void StateLoadingSavedGame::render() {
  this->context->t_engine->renderer.renderer2D.render(background);
  this->context->t_engine->renderer.renderer2D.render(loadingSlot);
  this->context->t_engine->renderer.renderer2D.render(loadingprogress);
  this->context->t_engine->renderer.renderer2D.render(loadingStateText);
}

void StateLoadingSavedGame::unload() {
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(
      *background);
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(
      *loadingSlot);
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(
      *loadingprogress);
  this->context->t_engine->renderer.getTextureRepository().freeBySprite(
      *loadingStateText);

  delete background;
  delete loadingSlot;
  delete loadingprogress;
  delete loadingStateText;
  delete worldOptions;
}

void StateLoadingSavedGame::createEntities() {
  this->stateGamePlay->world = new World(*worldOptions);
  this->stateGamePlay->player = new Player(
      &this->context->t_engine->renderer, this->context->t_soundManager,
      &this->stateGamePlay->world->blockManager);
  this->stateGamePlay->itemRepository = new ItemRepository();
  this->stateGamePlay->ui = new Ui();
  setPercent(25.0F);
  this->shouldCreatedEntities = 0;
}

void StateLoadingSavedGame::initItemRepository() {
  this->stateGamePlay->itemRepository->init(&this->context->t_engine->renderer,
                                            worldOptions->texturePack);

  setPercent(35.0F);
  this->shouldInitItemRepository = 0;
  TYRA_LOG("initItemRepository");
}

void StateLoadingSavedGame::initUI() {
  this->stateGamePlay->ui->init(&this->context->t_engine->renderer,
                                this->stateGamePlay->itemRepository,
                                this->stateGamePlay->player);
  setPercent(50.0F);
  this->shouldInitUI = 0;
  TYRA_LOG("initUI");
}

void StateLoadingSavedGame::initWorld() {
  this->stateGamePlay->world->init(&this->context->t_engine->renderer,
                                   this->stateGamePlay->itemRepository,
                                   this->context->t_soundManager);
  setPercent(70.0F);
  this->shouldInitWorld = 0;
  TYRA_LOG("initWorld");
}

void StateLoadingSavedGame::loadSavedData() {
  SaveManager::LoadSavedGame(this->stateGamePlay, saveFileFullPath.c_str());
  this->stateGamePlay->world->generateLight();
  this->stateGamePlay->world->loadSpawnArea();

  setPercent(80.0F);
  this->shouldLoadSavedData = 0;
  TYRA_LOG("loadSavedWorld");
}

void StateLoadingSavedGame::initPlayer() {
  this->stateGamePlay->player->mesh->getPosition()->set(
      this->stateGamePlay->world->getGlobalSpawnArea());
  this->stateGamePlay->player->spawnArea.set(
      this->stateGamePlay->world->getLocalSpawnArea());
  setPercent(100.0F);
  this->shouldInitPlayer = 0;
  TYRA_LOG("initPlayer");
}

void StateLoadingSavedGame::nextState() {
  TYRA_LOG("nextState");
  this->context->setState(this->stateGamePlay);
}

void StateLoadingSavedGame::setPercent(float completed) {
  this->_percent = completed;
  this->loadingprogress->size.set(this->_percent / 100 * 250, 9);
}

void StateLoadingSavedGame::setBgColorBlack() {
  this->context->t_engine->renderer.setClearScreenColor(
      Color(0.0F, 0.0F, 0.0F));
}

bool StateLoadingSavedGame::hasFinished() {
  return this->_state == LoadingState::Complete;
}
