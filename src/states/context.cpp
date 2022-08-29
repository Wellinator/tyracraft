#include "managers/state_manager.hpp"
#include "renderer/models/color.hpp"
#include "file/file_utils.hpp"
#include <renderer/core/2d/sprite/sprite.hpp>
#include <thread>
#include <chrono>

using Tyra::Audio;
using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;

Context::Context(Engine* t_engine, Camera* t_camera) {
  this->t_renderer = &t_engine->renderer;
  this->t_audio = &t_engine->audio;
  this->t_pad = &t_engine->pad;
  this->t_camera = t_camera;
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
