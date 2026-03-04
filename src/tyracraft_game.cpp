#include "tyracraft_game.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/settings_manager.hpp"
#include "memory-monitor/memory_monitor.hpp"
#include "utils.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <gs_privileged.h>

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

TyraCraftGame::~TyraCraftGame() {
  auto* bgService = BackgroundTaskService::getInstance();
  if (bgService) delete bgService;
}

void TyraCraftGame::init() {
  loadSavedSettings();
  checkNeededDirectories();
  engine->renderer.core.setFrameLimit(false);

  // Initialize background task service
  new BackgroundTaskService();

  // Test background task: simulates work on worker thread
  auto* bgService = BackgroundTaskService::getInstance();
  bgService->submit(
      []() {
        // Runs on worker thread
        printf("[BgTask Worker] Starting test task on thread %d\n",
               GetThreadId());
        volatile int sum = 0;
        for (int i = 0; i < 100000; i++) sum += i;
        printf("[BgTask Worker] Test task done. Result: %d\n", (int)sum);
      },
      []() {
        // Runs on main thread via pollCompletions()
        printf("[BgTask Main] Completion callback fired on thread %d\n",
               GetThreadId());
      });
}

void TyraCraftGame::loop() {
  timer.update();

  // Physics first (fixed timestep) - must run before variable update
  while (timer.updateFrame()) {
    stateManager.fixedUpdate(timer.getFixedDeltaTime());
  }

  // Variable update runs every iteration, after physics so state is up-to-date
  const float deltaTime = timer.getDeltaTime();
  stateManager.update(deltaTime);
  notificationManger.update(deltaTime);

  // Control render calls
  if (timer.renderFrame()) {
    engine->renderer.beginFrame(camera.getCameraInfo());

    stateManager.render();
    notificationManger.render();

#ifdef DEBUG_MODE
    // Draw FPS:
    const float renderFps =
        timer.getRenderMs() > 0.0f ? 1000.0f / timer.getRenderMs() : 0.0f;
    const float physicsFps = timer.getPhysicsUpdateMs() > 0.0f
                                 ? 1000.0f / timer.getPhysicsUpdateMs()
                                 : 0.0f;
    // FIX: getDeltaTimeAvg() returns seconds, not milliseconds
    // So FPS = 1 / seconds, not 1000 / seconds
    const float mainLoopFps = timer.getDeltaTimeAvg() > 0.0f
                                  ? 1.0f / timer.getDeltaTimeAvg()
                                  : 0.0f;

    std::stringstream stream;
    stream << "Render: " << std::fixed << std::setprecision(1) << renderFps
           << " fps  Physics: " << std::fixed << std::setprecision(1)
          << physicsFps << " fps  LoopHz: " << std::fixed
          << std::setprecision(1) << mainLoopFps;

    fontManager.printText(stream.str(),
                          FontOptions(Vec2(5.0f, 5.0f), Color(255), 0.6F));
    stream.str("");
    stream.clear();

    stream << "R: " << std::fixed << std::setprecision(2) << timer.getRenderMs()
           << "ms  P: " << std::fixed << std::setprecision(2)
           << timer.getPhysicsUpdateMs() << "ms  Mode: "
           << (timer.getIsInterlaced() ? "Interlaced" : "Progressive")
         << "  Field: " << (int)timer.getFieldCounter()
         << "  LastBit: " << (int)timer.getLastFieldBit();
    fontManager.printText(stream.str(),
                          FontOptions(Vec2(5.0f, 20.0f), Color(255), 0.6F));
                        stream.str("");
                        stream.clear();

                      // Diagnostic vsync info
                      u32 csrValue = *GS_REG_CSR;
                      stream << "VSync Debug: Calls=" << timer.getRenderFrameCalls()
                        << " Events=" << timer.getFieldToggleDetected()
                        << " CSR=0x" << std::hex << csrValue << std::dec
                        << " b3=" << ((csrValue >> 3) & 1)
                        << " b13=" << ((csrValue >> 13) & 1)
                        << " b12=" << ((csrValue >> 12) & 1);
                      fontManager.printText(stream.str(),
                             FontOptions(Vec2(5.0f, 35.0f), Color(255), 0.6F));

    // Draw Memory Usage:
    stream.str("");
    stream.clear();
    stream << "Memory : " << std::fixed << std::setprecision(3)
           << get_used_memory() / 1024.0f / 1024.0f << "MB / 32MB";
    fontManager.printText(stream.str(),
                          FontOptions(Vec2(5.0f, 50.0f), Color(255), 0.6F));
#endif

    engine->renderer.endFrame();
  }

  // Poll background task completions
  auto* bgService = BackgroundTaskService::getInstance();
  if (bgService) bgService->pollCompletions();

  // Use remaining idle CPU cycles for chunk loading work
  stateManager.processIdleWork();
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
