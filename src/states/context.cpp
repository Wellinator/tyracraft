#include "states/context.hpp"
#include <string>
#include <managers/font/font_manager.hpp>
#include <managers/font/font_options.hpp>
#include "memory-monitor/memory_monitor.hpp"

Context::Context(Engine* t_engine, Camera* t_camera) {
  this->t_engine = t_engine;
  this->t_camera = t_camera;
  t_soundManager = new SoundManager(t_engine);
}

Context::~Context() {
  delete state;
  delete t_soundManager;
}

void Context::update(const float& deltaTime) { state->update(deltaTime); }

void Context::render() {
  state->render();

  // Draw FPS:
  std::string fps =
      std::string("FPS: ").append(std::to_string(t_engine->info.getFps()));
  FontManager_printText(fps, FontOptions(Vec2(5.0f, 20.0f), Color(255), 0.8F));

  std::string memory = std::string("RAM: ").append(
      std::to_string(static_cast<float>(get_used_memory() / 1024)));
  FontManager_printText(memory,
                        FontOptions(Vec2(100.0f, 20.0f), Color(255), 0.8F));
}

void Context::setState(GameState* newState) {
  if (state != nullptr) delete state;
  state = newState;
}
