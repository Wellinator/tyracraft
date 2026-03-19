#include "states/game_play/states/survival/survival_playing_state.hpp"

SurvivalPlayingState::SurvivalPlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {}

SurvivalPlayingState::~SurvivalPlayingState() {}

void SurvivalPlayingState::init() {
  tickManager.onTick = [this]() { tick(); };
}

void SurvivalPlayingState::afterInit() {
  // World and player are fully initialized at this point
  stateGamePlay->world->setTickContext(stateGamePlay->player,
                                       stateGamePlay->context->t_camera);
  stateGamePlay->world->registerTickCallbacks(tickManager.scheduler);
  stateGamePlay->player->registerTickCallbacks(tickManager.scheduler);
}

void SurvivalPlayingState::update(const float& deltaTime) {
  tickManager.update(deltaTime);
}

void SurvivalPlayingState::tick() {}

void SurvivalPlayingState::render() {}

void SurvivalPlayingState::processIdleWork() {
  stateGamePlay->world->processIdleWork();
}

void SurvivalPlayingState::handleInput(const float& deltaTime) {}

void SurvivalPlayingState::navigate() {}
