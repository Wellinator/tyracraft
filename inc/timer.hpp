#pragma once

#include "tyra"
#include "constants.hpp"
#include "singleton.hpp"
#include <tamtypes.h>
#include <time.h>
#include <deque>

using Tyra::Engine;

class Timer : public Singleton<Timer> {
 public:
  void update();

  inline double getDeltaTime() { return realDeltaTime; };
  inline double getDeltaTimeAvg() { return avgDeltaTime; };
  inline bool skipFrame() { return _skipFrame; };

 private:
  u8 _skipFrame = false;
  clock_t begin = clock();

  float targetRenderFrame = FIXED_60_FRAME_MS;
  double accumulator = targetRenderFrame;
  double realDeltaTime = 0.0f;
  double avgDeltaTime = 0.0f;
  std::deque<float> dtDeque = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};
