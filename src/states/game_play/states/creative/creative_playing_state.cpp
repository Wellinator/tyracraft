#include "states/game_play/states/creative/creative_playing_state.hpp"

CreativePlayingState::CreativePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {}

CreativePlayingState::~CreativePlayingState() {}

void CreativePlayingState::init() {
  this->t_creativeAudioListener =
      new CreativeAudioListener(this->context->context);
  this->t_creativeAudioListener->playRandomCreativeSound();
  this->context->context->t_audio->song.addListener(
      this->t_creativeAudioListener);
}

void CreativePlayingState::update(const float& deltaTime) {
  Threading::switchThread();
  this->context->world->update(this->context->player,
                               this->context->context->t_camera,
                               this->context->context->t_pad, deltaTime);
  Threading::switchThread();
  this->context->player->update(deltaTime, *this->context->context->t_pad,
                                *this->context->context->t_camera,
                                this->context->world->getLoadedBlocks());
  Threading::switchThread();
  this->context->ui->update();
  Threading::switchThread();
  this->context->context->t_camera->update(*this->context->context->t_pad,
                                           *this->context->player->mesh);
}

void CreativePlayingState::render() {}

void CreativePlayingState::handleInput() {}

void CreativePlayingState::navigate() {}
