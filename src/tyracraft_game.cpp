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
    : notificationManger(&t_engine->renderer),
      fontManager(&t_engine->renderer),
      camera(engine->renderer.core.getSettings()),
      stateManager(t_engine, &camera) {
  engine = t_engine;
#ifdef DEBUG_MODE
  init_memory_manager();
#endif
}

TyraCraftGame::~TyraCraftGame() {}

void TyraCraftGame::init() {
  loadSavedSettings();
  checkNeededDirectories();

  engine->renderer.core.setFrameLimit(g_settings.vsync);
}

void TyraCraftGame::loop() {
  timer.update();

  if (timer.updateFrame()) {
    stateManager.fixedUpdate(timer.getFixedDeltaTime());
  }

  const double smoothedDeltaTime = timer.getDeltaTimeAvg();
  stateManager.update(smoothedDeltaTime);
  notificationManger.update(smoothedDeltaTime);

  // Control render calls
  if (timer.renderFrame()) {
    engine->renderer.beginFrame(camera.getCameraInfo());
    stateManager.render();
    notificationManger.render();

    // Draw FPS:
    std::stringstream stream;
    stream << "FPS: " << std::fixed << std::setprecision(2)
           << timer.getUpdateTime();
    fontManager.printText(stream.str(),
                          FontOptions(Vec2(10.0f, 10.0f), Color(255), 0.9F));
    stream.str("");
    stream.clear();

    stream << "Physics: " << std::fixed << std::setprecision(2)
           << timer.getPhysicsUpdateMs() << "ms Render: " << std::fixed
           << std::setprecision(2) << timer.getRenderMs() << "ms";
    fontManager.printText(stream.str(),
                          FontOptions(Vec2(10.0f, 30.0f), Color(255), 0.9F));
    engine->renderer.endFrame();
  }
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
