#include "timer.hpp"
#include <numeric>
#include <iostream>

namespace TyraCraft {

double Timer::stateLerp = 0.0f;

void Timer::update() {
  // Calc real delta time
  clock_t end = clock();
  realDeltaTime = double(end - begin) / double(CLOCKS_PER_SEC);
  begin = end;

  // Calc delta time average
  dtDeque.push_back(realDeltaTime);
  dtDeque.pop_front();
  double sum = std::accumulate(dtDeque.begin(), dtDeque.end(), 0.0f);
  avgDeltaTime = sum / 10.0f;

  // Update physics accumulator
  physicsAcc += realDeltaTime;
  _timeToUpdate = physicsAcc >= targetUpdateFrame;
  if (_timeToUpdate) {
    physicsAcc -= targetUpdateFrame;

    // Calc delta time to fixed update
    clock_t physicsEnd = clock();
    physicsMs = double(physicsEnd - physicsBegin) / double(CLOCKS_PER_SEC);
    physicsBegin = physicsEnd;
  }

  // Update render accumulator
  renderAcc += realDeltaTime;
  _timeToRender = renderAcc >= targetRenderFrame;
  if (_timeToRender) {
    renderAcc -= targetRenderFrame;

    // Calc delta time to render update
    clock_t renderEnd = clock();
    renderMs = double(renderEnd - renderBegin) / double(CLOCKS_PER_SEC);
    renderBegin = renderEnd;
  }

  Timer::stateLerp = physicsAcc / targetUpdateFrame;

  // using namespace std;
  // cout << "dt: " << realDeltaTime << " dt(avg): " << avgDeltaTime << endl;
}

}  // namespace TyraCraft