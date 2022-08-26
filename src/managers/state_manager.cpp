#include "managers/state_manager.hpp"

StateManager::StateManager(Engine* t_engine) {
  this->engine = t_engine;
  this->context = new Context(this->engine);
  this->context->state = new StateSplashScreen(context);
}

StateManager::~StateManager() {}

void StateManager::update(const float& deltaTime, Camera* t_camera) {
  this->context->update(deltaTime, t_camera);
}
