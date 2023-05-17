#include "tyracraft_game.hpp"
#include "engine.hpp"

int main(int argc, char* argv[]) {
  Tyra::EngineOptions options;

  options.loadUsbDriver = true;
  // options.writeLogsToFile = true;

  Tyra::Engine engine(options);
  TyraCraft::TyraCraftGame game(&engine);
  engine.run(&game);
  SleepThread();
  return 0;
}