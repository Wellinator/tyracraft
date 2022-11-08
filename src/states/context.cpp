#include "states/context.hpp"

Context::Context(Engine* t_engine, Camera* t_camera) {
  this->t_engine = t_engine;
  this->t_camera = t_camera;
  this->t_soundManager = new SoundManager(t_engine);
}

Context::~Context() { delete this->state; }

void Context::update(const float& deltaTime) {
  this->state->update(deltaTime);
  this->state->render();
}

void Context::setState(GameState* newState) {
  if (this->state != nullptr) delete this->state;
  this->state = newState;
}
