#include "tyracraft_game.hpp"

namespace TyraCraft {

using namespace Tyra;

TyraCraftGame::TyraCraftGame(Engine* t_engine)
    : camera(engine->renderer.core.getSettings()) {
  engine = t_engine;
}
TyraCraftGame::~TyraCraftGame() {}

void TyraCraftGame::init() {
  cameraPosition = Vec4(0.0F, 0.0F, 0.0F, 1.0F);
  cameraLookAt = Vec4(1.0F, 1.0F, 0.0F, 1.0F);
  stateManager.init(engine);
}

void TyraCraftGame::loop() {
  engine->renderer.beginFrame(CameraInfo3D(&camera.position, &camera.lookPos));
  printf("%i\n", engine->info.getFps());
  stateManager.update(engine, 1 / engine->info.getFps(), &engine->pad, &camera);
  engine->renderer.endFrame();
}

}  // namespace TyraCraft
