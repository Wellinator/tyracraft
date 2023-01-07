#include "tyracraft_game.hpp"

namespace TyraCraft {

using namespace Tyra;

TyraCraftGame::TyraCraftGame(Engine* t_engine)
    : camera(engine->renderer.core.getSettings()) {
  engine = t_engine;
}
TyraCraftGame::~TyraCraftGame() {}

void TyraCraftGame::init() { stateManager = new StateManager(engine, &camera); }

void TyraCraftGame::loop() {
  engine->renderer.beginFrame(camera.getCameraInfo());
  engine->renderer.core.setFrameLimit(false);
  stateManager->update(1 / static_cast<float>(engine->info.getFps()));
  engine->renderer.endFrame();
}

}  // namespace TyraCraft
