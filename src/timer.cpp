#include "timer.hpp"
#include <numeric>
#include <iostream>

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

  // Update accumulator
  accumulator += realDeltaTime;
  _skipFrame = accumulator < targetRenderFrame;
  if (!_skipFrame) {
    accumulator -= targetRenderFrame;
  }

  // using namespace std;
  // cout << "dt: " << realDeltaTime << " dt(avg): " << avgDeltaTime << endl;
}