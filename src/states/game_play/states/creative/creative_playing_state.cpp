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

void CreativePlayingState::update() {}

void CreativePlayingState::render() {}

void CreativePlayingState::handleInput() {}

void CreativePlayingState::navigate() {}
