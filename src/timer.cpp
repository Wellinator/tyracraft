#include "timer.hpp"
#include "managers/settings_manager.hpp"
#include <numeric>
#include <iostream>
#include <graph.h>

namespace TyraCraft {

float Timer::stateLerp = 0.0f;

void Timer::update() {
  // Calc real delta time - usando multiplicação ao invés de divisão
  const clock_t end = clock();
  realDeltaTime = static_cast<float>(end - begin) * INV_CLOCKS_PER_SEC;
  begin = end;

  // Track the number of timer iterations per second
  iteratorAcc += realDeltaTime;
  ++tempTimerIterationsCounter;
  if (iteratorAcc >= 1.0f) {
    iteratorAcc -= 1.0f;  // Preserva o excesso ao invés de zerar
    timerIterationsCounter = tempTimerIterationsCounter;
    tempTimerIterationsCounter = 0;
  }

  // Calc delta time average usando buffer circular (muito mais rápido que deque)
  dtSum -= dtSamples[dtIndex];  // Remove valor antigo da soma
  dtSamples[dtIndex] = realDeltaTime;  // Adiciona novo valor
  dtSum += realDeltaTime;  // Atualiza soma
  dtIndex = (dtIndex + 1) % 10;  // Avança índice circular
  avgDeltaTime = dtSum * INV_DT_SAMPLES;  // Multiplicação ao invés de divisão

  physicsAcc += realDeltaTime;
  renderAcc += realDeltaTime;

  Timer::stateLerp = physicsAcc * (1.0f / targetUpdateFrame);  // Pré-calcular se possível
}

// Calc delta time to fixed update
bool Timer::updateFrame() {
  const bool result = physicsAcc >= targetUpdateFrame;
  if (result) {
    physicsAcc -= targetUpdateFrame;
    const clock_t physicsEnd = clock();
    physicsMs = static_cast<float>(physicsEnd - physicsBegin) * INV_CLOCKS_PER_SEC * 1000.0f;
    physicsBegin = physicsEnd;

    // Skip frames if the physics accumulator is too high - usando multiplicação
    if (physicsAcc * (1.0f / targetUpdateFrame) > MAX_FRAME_SKIP) {
      physicsAcc = targetUpdateFrame * 0.5f;
    }
  }
  return result;
};

// Calc delta time to render update
bool Timer::renderFrame() {
  bool result;

  if (g_settings.vsync) {
    if (!is_waiting_vsync) {
      graph_start_vsync();
      is_waiting_vsync = true;
    }
    result = graph_check_vsync() != 0;
  } else {
    result = renderAcc >= targetRenderFrame;
  }

  if (result) {
    is_waiting_vsync = false;
    renderAcc -= targetRenderFrame;
    const clock_t renderEnd = clock();
    renderMs = static_cast<float>(renderEnd - renderBegin) * INV_CLOCKS_PER_SEC * 1000.0f;
    renderBegin = renderEnd;

    // Skip frames if the render accumulator is too high - usando multiplicação
    if (renderAcc * (1.0f / targetRenderFrame) > MAX_FRAME_SKIP) {
      renderAcc = targetRenderFrame * 0.5f;
    }
  }
  return result;
};

}  // namespace TyraCraft