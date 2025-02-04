#pragma once

#include "tyra"
#include "constants.hpp"
#include "singleton.hpp"
#include <tamtypes.h>
#include <time.h>
#include <deque>

using Tyra::Engine;

namespace TyraCraft {

class Timer : public Singleton<Timer> {
 public:
  void update();

  inline double getDeltaTime() { return realDeltaTime; };
  inline double getFixedDeltaTime() { return targetUpdateFrame; };
  inline double getDeltaTimeAvg() { return avgDeltaTime; };
  inline bool renderFrame() { return _timeToRender; };
  inline bool updateFrame() { return _timeToUpdate; };

  const double getRenderMs() { return renderMs * 1000; };
  const double getPhysicsUpdateMs() { return physicsMs * 1000; };

  static double stateLerp;

 private:
  u8 _timeToRender = false, _timeToUpdate = false;
  clock_t begin = clock();

  float targetRenderFrame = FIXED_60_FRAME_MS,
        targetUpdateFrame = FIXED_30_FRAME_MS;

  double renderAcc = targetRenderFrame;
  double physicsAcc = targetRenderFrame;

  clock_t renderEnd, renderBegin = clock();
  double renderMs = 0.0f;
  clock_t physicsEnd, physicsBegin = clock();
  double physicsMs = 0.0f;

  double realDeltaTime = 0.0f;
  double avgDeltaTime = 0.0f;
  std::deque<float> dtDeque = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

}  // namespace TyraCraft
