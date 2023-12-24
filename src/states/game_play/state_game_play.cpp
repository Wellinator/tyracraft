#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/in_game_menu/state_game_menu.hpp"
#include "states/game_play/states/welcome/state_welcome.hpp"
#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"
#include "managers/save_manager.hpp"
#include "managers/collision_manager.hpp"

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer;
using Tyra::Renderer3D;
using Tyra::RendererSettings;

StateGamePlay::StateGamePlay(Context* context, const GameMode& gameMode)
    : GameState(context) {
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
  if (gameMode == GameMode::Creative) {
    this->setPlayingState(new CreativePlayingState(this));
  } else if (gameMode == GameMode::Survival) {
    this->setPlayingState(new SurvivalPlayingState(this));
  }
}

void StateGamePlay::init() {
  // TODO: add in game skybox;
  this->state->init();
  displayWelcome();
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
  this->state = new StateWelcome(this);
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
  std::string saveFileName = FileUtils::fromCwd(
      "saves/" + this->world->getWorldOptions()->name + ".tcw");
  SaveManager::SaveGame(this, saveFileName.c_str());
  TYRA_LOG("Saving at: ", saveFileName.c_str());
}