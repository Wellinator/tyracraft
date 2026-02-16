#include "tyracraft_game.hpp"
#include "engine.hpp"
#include "screen_settings.hpp"

int main(int argc, char* argv[]) {
  Tyra::EngineOptions options;
  options.loadUsbDriver = true;
  options.loadMemoryCardDriver = false;
  options.customGraphycsSettings = Tyra::RendererSettings(
      SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_NEAR_PLANE, SCREEN_FAR_PLANE);

  Tyra::Engine engine(options);
  TyraCraft::TyraCraftGame game(&engine);
  engine.run(&game);
  SleepThread();
  return 0;
}