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

  inline u32 getUpdateTime() const { return timerIterationsCounter; }
  inline float getDeltaTime() const { return realDeltaTime; }
  inline float getFixedDeltaTime() const { return targetUpdateFrame; }
  inline float getDeltaTimeAvg() const { return avgDeltaTime; }
  inline float getRenderMs() const { return renderMs; }
  inline float getPhysicsUpdateMs() const { return physicsMs; }

  static float stateLerp;

 private:
  // Constantes pré-calculadas
  static constexpr float INV_CLOCKS_PER_SEC = 1.0f / CLOCKS_PER_SEC;
  static constexpr float INV_DT_SAMPLES = 1.0f / 10.0f;
  static constexpr u8 MAX_FRAME_SKIP = 2;
  
  // Membros ordenados por tamanho para melhor cache alignment
  clock_t begin = clock();
  clock_t renderBegin = clock();
  clock_t physicsBegin = clock();
  
  float targetRenderFrame = FIXED_60_FRAME_MS;
  float targetUpdateFrame = FIXED_30_FRAME_MS;
  float renderAcc = FIXED_60_FRAME_MS * 0.5f;
  float physicsAcc = FIXED_30_FRAME_MS * 0.5f;
  
  float iteratorAcc = 0.0f;
  float realDeltaTime = 0.0f;
  float avgDeltaTime = 0.0f;
  float renderMs = 0.0f;
  float physicsMs = 0.0f;
  float dtSum = 0.0f;  // Cache para soma ao invés de recalcular
  
  u32 timerIterationsCounter = 0;
  u32 tempTimerIterationsCounter = 0;
  u8 dtIndex = 0;  // Índice circular para array ao invés de deque
  
  bool is_waiting_vsync = false;
  
  // Array ao invés de deque para melhor performance
  float dtSamples[10] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

}  // namespace TyraCraft
