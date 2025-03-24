#include "timer.hpp"
#include <numeric>
#include <iostream>

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

  // Update physics accumulator
  physicsAcc += realDeltaTime;
  _timeToUpdate = physicsAcc >= targetUpdateFrame;
  if (_timeToUpdate) {
    physicsAcc = 0.0f;  //-= targetUpdateFrame;

    // Calc delta time to fixed update
    clock_t physicsEnd = clock();
    physicsMs = float(physicsEnd - physicsBegin) / float(CLOCKS_PER_SEC);
    physicsBegin = physicsEnd;
  }

  // Update render accumulator
  renderAcc += realDeltaTime;
  _timeToRender = renderAcc >= targetRenderFrame;
  if (_timeToRender) {
    renderAcc = 0.0f;  //-= targetRenderFrame;

    // Calc delta time to render update
    clock_t renderEnd = clock();
    renderMs = float(renderEnd - renderBegin) / float(CLOCKS_PER_SEC);
    renderBegin = renderEnd;
  }

  Timer::stateLerp = physicsAcc / targetUpdateFrame;

  // using namespace std;
  // cout << "dt: " << realDeltaTime << " dt(avg): " << avgDeltaTime << endl;
}

}  // namespace TyraCraft