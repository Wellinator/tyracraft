#include "states/game_play/state_game_play.hpp"
#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"

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
  this->context->t_audio->song.stop();
  this->context->t_audio->song.inLoop = false;
  delete this->world;
  delete this->ui;
  delete this->player;
  delete this->itemRepository;
  delete this->state;
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
  this->state->update(deltaTime);
}

void StateGamePlay::render() {
  this->state->render();
  this->world->render();
  this->player->render();
  this->ui->render();
}

void StateGamePlay::setPlayingState(PlayingStateBase* t_playingState) {
  delete this->state;
  this->state = t_playingState;
}
