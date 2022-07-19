#include "tyracraft_game.hpp"

namespace TyraCraft {

using namespace Tyra;

TyraCraftGame::TyraCraftGame(Engine* t_engine) { engine = t_engine; }
TyraCraftGame::~TyraCraftGame() {}

void TyraCraftGame::init() { TYRA_LOG("Hello!"); }

void TyraCraftGame::loop() {
  TYRA_LOG("Loop!");

  // engine->renderer.beginFrame(CameraInfo3D(&cameraPosition, &cameraLookAt));
  // {}
  // engine->renderer.endFrame();
}

}  // namespace TyraCraft
