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

  engine->renderer.core.setFrameLimit(false);
  printf("FPS: %i \n", engine->info.getFps());
  // printf("Free RAM: %f MB\n", engine->info.getAvailableRAM());

  stateManager.update(1 / static_cast<float>(engine->info.getFps()), &camera);
  engine->renderer.endFrame();
}

}  // namespace TyraCraft
