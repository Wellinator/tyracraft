#include "tyracraft_game.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/settings_manager.hpp"
#include "memory-monitor/memory_monitor.hpp"
#include "utils.hpp"
#include <sys/types.h>
#include <sys/stat.h>

namespace TyraCraft {

using namespace Tyra;

TyraCraftGame::TyraCraftGame(Engine* t_engine)
    : camera(engine->renderer.core.getSettings()),
      stateManager(t_engine, &camera) {
  engine = t_engine;
  init_memory_manager();
}

TyraCraftGame::~TyraCraftGame() {}

void TyraCraftGame::init() {
  loadSavedSettings();
  checkNeededDirectories();

  engine->renderer.core.setFrameLimit(g_settings.vsync);

  FontManager_init(&engine->renderer);
}

void TyraCraftGame::loop() {
  const float dt = 1 / static_cast<float>(engine->info.getFps());
  stateManager.update(dt);

  engine->renderer.beginFrame(camera.getCameraInfo());
  stateManager.render();
  engine->renderer.endFrame();
}

void TyraCraftGame::loadSavedSettings() {
  if (SettingsManager::CheckIfSettingsExist()) SettingsManager::Load();
}

void TyraCraftGame::checkNeededDirectories() { checkSavesDir(); }

void TyraCraftGame::checkSavesDir() {
  struct stat info;
  auto pathname = FileUtils::fromCwd("saves/");
  if (stat(pathname.c_str(), &info) != 0)
    TYRA_WARN("Can't access: ", pathname.c_str());
  if (info.st_mode & S_IFDIR) {
    TYRA_LOG("Save dir already exist. Skiping...");
  } else {
    TYRA_WARN("Creating Save directory...");
    mode_t mode = 0755;
    mkdir(pathname.c_str(), mode);
  }
}

}  // namespace TyraCraft
