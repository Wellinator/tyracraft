#include "tyracraft_game.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/settings_manager.hpp"
#include "memory-monitor/memory_monitor.hpp"
#include "utils.hpp"
#include "services/memory_card_service.hpp"
#include "services/network_service.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <gs_privileged.h>
#include "debug.hpp"

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

#ifdef DEBUG_MODE
  new TyraCraft::DebugLogger();
#endif

  // Initialize background task service
  new BackgroundTaskService();

  // Initialize memory card service
  new MemoryCardService();
  auto* mcService = MemoryCardService::getInstance();
  mcService->init();

  // Initialize network service
  if (g_settings.enable_log_over_lan) {
    new NetworkService();
    auto* networkService = NetworkService::getInstance();
    if (networkService->init()) {
      networkService->installExceptionHandler();
      networkService->testConnection(); // Run initial connectivity test
    }
  }


  // Memory Card Test
  if (mcService->isAvailable(0, 0)) {
    int freeKB = mcService->getFreeSpace(0, 0);
    TYRA_LOG("TEST: Memory Card detected in Slot 0! Free space: ", freeKB, " KB");
    mcService->ensureDirectoryExists(0, 0);
  } else {
    TYRA_LOG("TEST: No Memory Card detected in Slot 0.");
  }

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
    const float mainLoopFps =
        timer.getDeltaTimeAvg() > 0.0f ? 1.0f / timer.getDeltaTimeAvg() : 0.0f;

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
           << timer.getPhysicsUpdateMs();
    fontManager.printText(stream.str(),
                          FontOptions(Vec2(5.0f, 20.0f), Color(255), 0.6F));
    stream.str("");
    stream.clear();

    // Draw Memory Usage:
    stream.str("");
    stream.clear();
    stream << "Memory : " << std::fixed << std::setprecision(3)
           << get_used_memory() / 1024.0f / 1024.0f << "MB / 32MB";
    fontManager.printText(stream.str(),
                          FontOptions(Vec2(5.0f, 35.0f), Color(255), 0.6F));
    // Draw Network status:
    auto* networkService = NetworkService::getInstance();
    if (networkService && g_settings.enable_log_over_lan) {
      stream.str("");
      stream.clear();
      stream << "Network: " << networkService->getStatusString() << " ("
             << networkService->getIpAddress() << ")";
      fontManager.printText(
          stream.str(),
          FontOptions(Vec2(5.0f, 50.0f), networkService->getStatusColor(),
                      0.6F));
    }
#endif

    engine->renderer.endFrame();
  }

  // Poll background task completions
  auto* bgService = BackgroundTaskService::getInstance();
  if (bgService) bgService->pollCompletions();

  // Use remaining idle CPU cycles for chunk loading work
  stateManager.processIdleWork();

  // Update network status
  auto* networkService = NetworkService::getInstance();
  if (networkService && g_settings.enable_log_over_lan) networkService->update();
}


void TyraCraftGame::loadSavedSettings() {
  if (SettingsManager::CheckIfSettingsExist()) SettingsManager::Load();
}

void TyraCraftGame::checkNeededDirectories() { checkSavesDir(); }

void TyraCraftGame::checkSavesDir() {
  auto pathname = FileUtils::fromCwd("saves/");
  if (Utils::directoryExists(pathname)) {
    TCLOG("Save dir already exists. Skipping...");
    return;
  }

  TCLOG("Creating Save directory: %s", pathname.c_str());
  if (!Utils::makeDirectoryRecursive(pathname)) {
    TYRA_ERROR("Failed to create Save directory!");
  }
}

}  // namespace TyraCraft
