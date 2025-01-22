#include "tyracraft_game.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/settings_manager.hpp"
#include "memory-monitor/memory_monitor.hpp"
#include "utils.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <time.h>
#include <deque>

namespace TyraCraft {

clock_t begin = clock();
float fixedDeltaTime = 0.016f;
float accumulator = 0;
std::deque<float> dq = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

using namespace Tyra;

TyraCraftGame::TyraCraftGame(Engine* t_engine)
    : camera(engine->renderer.core.getSettings()),
      fontManager(&t_engine->renderer),
      notificationManger(&t_engine->renderer),
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
  using namespace std;

  clock_t end = clock();
  const float tempDt = float(end - begin) / double(CLOCKS_PER_SEC);
  begin = end;

  // Calc AVG
  dq.push_front(tempDt);
  dq.pop_back();

  float sum = std::accumulate(dq.begin(), dq.end(), 0.0f);
  const float dt = sum / 10.0f;

  notificationManger.update(dt);
  stateManager.update(dt);

  engine->renderer.beginFrame(camera.getCameraInfo());
  stateManager.render();
  notificationManger.render();
  engine->renderer.endFrame();

  // cout << "dt: " << tempDt << " dt(avg): " << dt << endl;
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
