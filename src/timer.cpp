#include "timer.hpp"
#include "managers/settings_manager.hpp"
#include <numeric>
#include <iostream>
#include <graph.h>

namespace TyraCraft {

float Timer::stateLerp = 0.0f;

void Timer::update() {
  // Calc real delta time
  clock_t end = clock();
  realDeltaTime = float(end - begin) / float(CLOCKS_PER_SEC);
  begin = end;

  // Track the number of timer iterations per second
  iteratorAcc += realDeltaTime;
  tempTimerIterationsCounter++;
  if (iteratorAcc >= 1.0f) {
    iteratorAcc = 0.0f;
    timerIterationsCounter = tempTimerIterationsCounter;
    tempTimerIterationsCounter = 0;
  }

  // Calc delta time average
  dtDeque.pop_front();
  dtDeque.push_back(realDeltaTime);
  float sum = std::accumulate(dtDeque.begin(), dtDeque.end(), 0.0f);
  avgDeltaTime = sum / 10.0f;

  physicsAcc += realDeltaTime;
  renderAcc += realDeltaTime;

  Timer::stateLerp = physicsAcc / targetUpdateFrame;

  // using namespace std;
  // cout << "dt: " << realDeltaTime << " dt(avg): " << avgDeltaTime << endl;
}

// Calc delta time to fixed update
bool Timer::updateFrame() {
  const bool result = physicsAcc >= targetUpdateFrame;
  if (result) {
    physicsAcc -= targetUpdateFrame;
    clock_t physicsEnd = clock();
    physicsMs = float(physicsEnd - physicsBegin) / float(CLOCKS_PER_SEC);
    physicsBegin = physicsEnd;

    // Skip frames if the physics accumulator is too high
    if (static_cast<int>(physicsAcc / targetUpdateFrame) > 2) {
      physicsAcc = targetUpdateFrame / 2.0f;
    }
  }
  return result;
};

// Calc delta time to render update
bool Timer::renderFrame() {
  bool result = false;

  if (g_settings.vsync) {
    if (is_waiting_vsync == false) {
      graph_start_vsync();
      is_waiting_vsync = true;
    }

    int check = graph_check_vsync();
    result = check != 0;
  } else {
    result = renderAcc >= targetRenderFrame;
  }

  if (result) {
    is_waiting_vsync = false;
    renderAcc -= targetRenderFrame;
    clock_t renderEnd = clock();
    renderMs = float(renderEnd - renderBegin) / float(CLOCKS_PER_SEC);
    renderBegin = renderEnd;

    // Skip frames if the render accumulator is too high
    if (static_cast<int>(renderAcc / targetRenderFrame) > 2) {
      renderAcc = targetRenderFrame / 2.0f;
    }
  }
  return result;
};

}  // namespace TyraCraft