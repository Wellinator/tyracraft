#include "tyracraft_game.hpp"
#include "engine.hpp"

int main(int argc, char* argv[]) {
  Tyra::EngineOptions options;
  options.loadUsbDriver = true;
  options.customGraphycsSettings =
      Tyra::RendererSettings(512.0f, 448.0f, 1.0f, 2500.0f);

  Tyra::Engine engine(options);
  TyraCraft::TyraCraftGame game(&engine);
  engine.run(&game);
  SleepThread();
  return 0;
}