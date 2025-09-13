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
  bool renderFrame();
  bool updateFrame();

  inline u32 getUpdateTime() { return timerIterationsCounter; };
  inline float getDeltaTime() { return realDeltaTime; };
  inline float getFixedDeltaTime() { return targetUpdateFrame; };
  inline float getDeltaTimeAvg() { return avgDeltaTime; };
  const float getRenderMs() { return renderMs * 1000; };
  const float getPhysicsUpdateMs() { return physicsMs * 1000; };

  static float stateLerp;

 private:
  bool is_waiting_vsync = false;
  clock_t begin = clock();

  float targetRenderFrame = FIXED_60_FRAME_MS,
        targetUpdateFrame = FIXED_30_FRAME_MS;

  float iteratorAcc = 0.0f;
  u32 timerIterationsCounter = 0, tempTimerIterationsCounter = 0;

  float renderAcc = targetRenderFrame / 2.0f;
  float physicsAcc = targetUpdateFrame / 2.0f;

  clock_t renderEnd, renderBegin = clock();
  float renderMs = 0.0f;
  clock_t physicsEnd, physicsBegin = clock();
  float physicsMs = 0.0f;

  float realDeltaTime = 0.0f;
  float avgDeltaTime = 0.0f;
  std::deque<float> dtDeque = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

}  // namespace TyraCraft
