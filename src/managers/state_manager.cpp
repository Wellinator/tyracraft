#include "managers/state_manager.hpp"

StateManager::StateManager(Engine* t_engine, Camera* t_camera) {
  context = new Context(t_engine, t_camera);
  context->setState(new StateSplashScreen(context));
}

StateManager::~StateManager() { delete context; }

void StateManager::fixedUpdate(const float& fixedDeltaTime) {
  context->fixedUpdate(fixedDeltaTime);
}

void StateManager::update(const float& deltaTime) {
  context->update(deltaTime);
}

void StateManager::render() { context->render(); }

void StateManager::processIdleWork() { context->processIdleWork(); }
