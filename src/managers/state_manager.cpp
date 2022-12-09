#include "managers/state_manager.hpp"

StateManager::StateManager(Engine* t_engine, Camera* t_camera) {
  this->context = new Context(t_engine, t_camera);
  this->context->setState(new StateSplashScreen(this->context));
}

StateManager::~StateManager() { delete this->context; }

void StateManager::update(const float& deltaTime) {
  tickManager.update(std::min(deltaTime, MAX_FRAME_MS));
  this->context->update(deltaTime);
}
