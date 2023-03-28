#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/in_game_menu/state_game_menu.hpp"
#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"
#include "managers/save_manager.hpp"

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
  delete this->state;
  delete this->previousState;
  delete this->world;
  delete this->ui;
  delete this->player;
  delete this->itemRepository;
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
  if (this->context->t_engine->pad.getClicked().Start) {
    if (this->paused)
      this->unpauseGame();
    else
      this->pauseGame();
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

void StateGamePlay::quitToTitle() {
  this->context->setState(new StateMainMenu(this->context));
}

void StateGamePlay::saveGame() {
  std::string saveFileName = FileUtils::fromCwd(
      "saves/" + this->world->getWorldOptions()->name + ".tcw");
  SaveManager::SaveGame(this, saveFileName.c_str());
  TYRA_LOG("Saving at: ", saveFileName.c_str());
}