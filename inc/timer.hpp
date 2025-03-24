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

  inline u32 getUpdateTime() { return timerIterationsCounter; };
  inline float getDeltaTime() { return realDeltaTime; };
  inline float getFixedDeltaTime() { return targetUpdateFrame; };
  inline float getDeltaTimeAvg() { return avgDeltaTime; };
  inline bool renderFrame() { return _timeToRender; };
  inline bool updateFrame() { return _timeToUpdate; };

  const float getRenderMs() { return renderMs * 1000; };
  const float getPhysicsUpdateMs() { return physicsMs * 1000; };

  static float stateLerp;

 private:
  u8 _timeToRender = false, _timeToUpdate = false;
  clock_t begin = clock();

  float targetRenderFrame = FIXED_60_FRAME_MS,
        targetUpdateFrame = FIXED_30_FRAME_MS;

  float iteratorAcc = 0.0f;
  u32 timerIterationsCounter = 0, tempTimerIterationsCounter = 0;

  float renderAcc = targetRenderFrame;
  float physicsAcc = targetUpdateFrame;

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
