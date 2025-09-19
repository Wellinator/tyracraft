#include "states/context.hpp"
#include <string>
#include <managers/font/font_manager.hpp>
#include <managers/font/font_options.hpp>

Context::Context(Engine* t_engine, Camera* t_camera) : soundManager(t_engine) {
  this->t_engine = t_engine;
  this->t_camera = t_camera;
}

Context::~Context() { delete state; }

void Context::fixedUpdate(const float& fixedDeltaTime) {
  state->fixedUpdate(fixedDeltaTime);
}

void Context::update(const float& deltaTime) { state->update(deltaTime); }

void Context::render() { state->render(); }

void Context::setState(GameState* newState) {
  if (state) delete state;
  state = newState;
}
