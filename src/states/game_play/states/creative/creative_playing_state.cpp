#include "states/game_play/states/creative/creative_playing_state.hpp"

CreativePlayingState::CreativePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {}

CreativePlayingState::~CreativePlayingState() {
  this->stateGamePlay->context->t_audio->song.removeListener(
      this->audioListenerId);
  delete this->t_creativeAudioListener;
}

void CreativePlayingState::init() {
  this->t_creativeAudioListener =
      new CreativeAudioListener(this->stateGamePlay->context);
  this->t_creativeAudioListener->playRandomCreativeSound();
  this->audioListenerId =
      this->stateGamePlay->context->t_audio->song.addListener(
          this->t_creativeAudioListener);
}

void CreativePlayingState::update(const float& deltaTime) {
  Threading::switchThread();
  this->stateGamePlay->world->update(
      this->stateGamePlay->player, this->stateGamePlay->context->t_camera,
      this->stateGamePlay->context->t_pad, deltaTime);
  Threading::switchThread();
  this->stateGamePlay->player->update(
      deltaTime, *this->stateGamePlay->context->t_pad,
      *this->stateGamePlay->context->t_camera,
      this->stateGamePlay->world->getLoadedBlocks());
  Threading::switchThread();
  this->stateGamePlay->ui->update();
  Threading::switchThread();
  this->stateGamePlay->context->t_camera->update(
      *this->stateGamePlay->context->t_pad, *this->stateGamePlay->player->mesh);
}

void CreativePlayingState::render() {
  this->stateGamePlay->world->render();
  this->stateGamePlay->player->render();
  this->renderCreativeUi();
}

void CreativePlayingState::handleInput() {}

void CreativePlayingState::navigate() {}

void CreativePlayingState::renderCreativeUi() {
  this->stateGamePlay->ui->renderCrosshair();
  this->stateGamePlay->ui->renderInventory();
}
