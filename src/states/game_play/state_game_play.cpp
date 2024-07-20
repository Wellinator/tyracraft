#include "states/game_play/state_game_play.hpp"
#include "states/loading/mini_games/state_load_next_level_maze_craft.hpp"
#include "states/loading/mini_games/state_create_maze_craft.hpp"
#include "states/game_play/states/in_game_menu/state_game_menu.hpp"
#include "states/game_play/states/welcome/state_welcome.hpp"
#include "states/game_play/states/minigame/mazecraft/mazecraft_level_init.hpp"
#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"
#include "managers/collision_manager.hpp"

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer;
using Tyra::Renderer3D;
using Tyra::RendererSettings;

StateGamePlay::StateGamePlay(Context* context, const GameMode& gameMode)
    : GameState(context), _gameMode(gameMode) {
  this->handleGameMode(gameMode);
  this->init();
}

StateGamePlay::~StateGamePlay() {
  this->context->t_engine->audio.song.stop();
  this->context->t_engine->audio.song.inLoop = false;

  TYRA_LOG("freeing state...");
  delete this->state;
  TYRA_LOG("freeing previousState...");
  delete this->previousState;
  TYRA_LOG("freeing world...");
  delete this->world;
  TYRA_LOG("freeing ui...");
  delete this->ui;
  TYRA_LOG("freeing player...");
  delete this->player;
  TYRA_LOG("freeing itemRepository...");
  delete this->itemRepository;
  TYRA_LOG("freeing completed!");

  CollisionManager_unloadTree();
}

void StateGamePlay::handleGameMode(const GameMode& gameMode) {
  switch (gameMode) {
    case GameMode::Creative:
      this->setPlayingState(new CreativePlayingState(this));
      break;
    case GameMode::Survival:
      this->setPlayingState(new SurvivalPlayingState(this));
      break;
    case GameMode::Maze:
      this->setPlayingState(new MazePlayingState(this));
      break;

    default:
      TYRA_TRAP("Invalid handleGameMode");
      break;
  }
}

void StateGamePlay::init() {
  // TODO: add in game skybox;
  this->state->init();
  displayWelcome();
}

void StateGamePlay::afterInit() {
  if (previousState != nullptr) previousState->afterInit();
  this->state->afterInit();
}

void StateGamePlay::update(const float& deltaTime) {
  this->handleInput();
  this->state->update(deltaTime);
}

void StateGamePlay::render() { this->state->render(); }

void StateGamePlay::setPlayingState(PlayingStateBase* t_playingState) {
  delete this->state;
  this->state = t_playingState;
}

PlayingStateBase* StateGamePlay::getPreviousState() {
  return this->previousState;
}

void StateGamePlay::handleInput() {
  const PadButtons& clicked = this->context->t_engine->pad.getClicked();

  if (!isAtWelcomeState && clicked.Start) {
    if (this->paused)
      this->unpauseGame();
    else
      this->pauseGame();

  } else if (isAtWelcomeState && (clicked.Cross || clicked.Start)) {
    hideWelcome();
  }
}

void StateGamePlay::pauseGame() {
  this->previousState = this->state;
  this->state = new StateGameMenu(this);
  this->paused = true;
}

void StateGamePlay::unpauseGame() {
  delete this->state;
  this->state = this->previousState;
  this->previousState = nullptr;
  this->paused = false;
}

void StateGamePlay::displayWelcome() {
  this->previousState = this->state;

  switch (_gameMode) {
    case GameMode::Maze:
      this->state = new MazecraftLevelInit(this);
      break;

    default:
      this->state = new StateWelcome(this);
      break;
  }

  this->isAtWelcomeState = true;
}

void StateGamePlay::hideWelcome() {
  delete this->state;
  this->state = this->previousState;
  this->previousState = nullptr;
  this->isAtWelcomeState = false;
}

void StateGamePlay::quitToTitle() {
  if (paused) unpauseGame();
  context->setState(new StateMainMenu(context));
}

void StateGamePlay::saveGame() {
  this->previousState->handleAction(MenuAction::Save);
}

void StateGamePlay::loadNextMiniGameLevel(const NewGameOptions& options) {
  TYRA_LOG("loadNextMiniGameLevel()");
  context->setState(new StateLoadNextLevelMazeCraft(context, options));
}