#include "states/game_play/states/state_game_menu.hpp"

StateGameMenu::StateGameMenu(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {
  this->init();
}

StateGameMenu::~StateGameMenu() {}

void StateGameMenu::init() {}

void StateGameMenu::update(const float& deltaTime) {}

void StateGameMenu::render() {
  this->stateGamePlay->getPreviousState()->render();
}

void StateGameMenu::handleInput() {}

void StateGameMenu::navigate() {}

void StateGameMenu::unloadTextures() {}
