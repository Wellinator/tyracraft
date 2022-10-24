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
using Tyra::Threading;

StateGamePlay::StateGamePlay(Context* context, const GameMode& gameMode)
    : GameState(context) {
  // Define GAME_MODE
  // TODO: refactor to a function
  {
    if (gameMode == GameMode::Creative) {
      this->setPlayingState(new CreativePlayingState(this));
    } else if (gameMode == GameMode::Survival) {
      this->setPlayingState(new SurvivalPlayingState(this));
    }
  }
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

void StateGamePlay::init() {
  // TODO: add in game skybox;

  this->t_creativeAudioListener = new CreativeAudioListener(this->context);
  this->t_creativeAudioListener->playRandomCreativeSound();
  this->context->t_audio->song.addListener(this->t_creativeAudioListener);
}

void StateGamePlay::update(const float& deltaTime) {
  this->state->update();

  this->handleInput();
  Threading::switchThread();
  this->world->update(this->player, this->context->t_camera,
                      this->context->t_pad, deltaTime);
  Threading::switchThread();
  this->player->update(deltaTime, *this->context->t_pad,
                       *this->context->t_camera,
                       this->world->getLoadedBlocks());
  Threading::switchThread();
  this->ui->update();
  Threading::switchThread();
  this->context->t_camera->update(*this->context->t_pad, *this->player->mesh);
}

void StateGamePlay::render() {
  this->state->render();
  this->world->render();
  this->player->render();
  this->ui->render();
}

void StateGamePlay::handleInput() { this->controlGameMode(); }

// TODO : Check the delay to change
void StateGamePlay::controlGameMode() {
  if (this->context->t_pad->getPressed().DpadUp &&
      this->context->t_pad->getClicked().Cross) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            this->lastTimeCrossWasClicked - std::chrono::steady_clock::now())
            .count() <= 500) {
      printf("GAME_MODE changed to %d\n", (u8)this->gameMode);
      this->gameMode = this->gameMode == GameMode::Creative
                           ? GameMode::Survival
                           : GameMode::Creative;
    }
    this->lastTimeCrossWasClicked = std::chrono::steady_clock::now();
  }
}

void StateGamePlay::setPlayingState(PlayingStateBase* t_playingState) {
  delete this->state;
  this->state = t_playingState;
}
