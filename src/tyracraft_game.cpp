#include "tyracraft_game.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/settings_manager.hpp"
#include "utils.hpp"
#include <sys/types.h>
#include <sys/stat.h>

namespace TyraCraft {

using namespace Tyra;

TyraCraftGame::TyraCraftGame(Engine* t_engine)
    : camera(engine->renderer.core.getSettings()) {
  engine = t_engine;
}

TyraCraftGame::~TyraCraftGame() {}

void TyraCraftGame::init() {
  loadSavedSettings();
  checkNeededDirectories();

  engine->renderer.core.setFrameLimit(g_settings.vsync);

  FontManager_init(&engine->renderer);
  stateManager = new StateManager(engine, &camera);
}

void TyraCraftGame::loop() {
  engine->renderer.beginFrame(camera.getCameraInfo());
  const auto dt = std::min(1 / static_cast<float>(engine->info.getFps()),
                           FIXED_30_FRAME_MS);
  stateManager->update(dt);
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
