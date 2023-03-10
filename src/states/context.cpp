#include "states/context.hpp"

Context::Context(Engine* t_engine, Camera* t_camera) {
  this->t_engine = t_engine;
  this->t_camera = t_camera;
  t_soundManager = new SoundManager(t_engine);
}

Context::~Context() {
  delete state;
  delete t_soundManager;
}

void Context::update(const float& deltaTime) {
  state->update(deltaTime);
  state->render();
}

void Context::setState(GameState* newState) {
  if (state != nullptr) delete state;
  state = newState;
}
