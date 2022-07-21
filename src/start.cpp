#include "start.hpp"

Start::Start(Engine* t_engine) : camera(engine->renderer.core.getSettings()) {
  engine = t_engine;
}

Start::~Start() { return; }

void Start::init() { stateManager.init(&engine->renderer, &engine->audio); }

void Start::loop() {
  // printf("%f\n", engine->fps);
  engine->renderer.beginFrame(CameraInfo3D(&camera.position, &camera.lookPos));
  stateManager.update(1 / engine->info.getFps(), engine->pad, camera);
}