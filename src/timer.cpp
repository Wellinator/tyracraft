#include "timer.hpp"
#include "managers/settings_manager.hpp"
#include <graph.h>
#include <gs_privileged.h>

namespace TyraCraft {

float Timer::stateLerp = 0.0f;

void Timer::init() {
    lastCpuCount = getCpuCycles();
    
    // Set initial target based on persisted setting
    setFpsMode(g_settings.fps_mode);
    
    physicsAcc = TARGET_PHYSICS_CYCLES / 2;
    
    // Initialize start times for the first frame interval measurement
    u32 now = getCpuCycles();
    renderBeginCpuCount = now;
    physicsBeginCpuCount = now;
}

void Timer::update() {
    u32 currentCpuCount = getCpuCycles();
    
    // Calculate cycles passed since last update
    // Handles 32-bit overflow naturally with unsigned arithmetic
    u32 cyclesPassed = currentCpuCount - lastCpuCount;
    lastCpuCount = currentCpuCount;

    // --- Stats & Getters Update ---
    
    // Convert current delta to seconds (float) for gameplay usage
    static constexpr float INV_CYCLES_PER_SEC = 1.0f / static_cast<float>(EE_CYCLES_PER_SEC);
    realDeltaTime = static_cast<float>(cyclesPassed) * INV_CYCLES_PER_SEC;

    // Clean average calculation using integer sum
    dtSum -= dtSamples[dtIndex];
    dtSamples[dtIndex] = cyclesPassed;
    dtSum += cyclesPassed;
    dtIndex = (dtIndex + 1) % 10;
    
    // Avg in seconds
    static constexpr float INV_SAMPLES_COUNT = 1.0f / 10.0f;
    avgDeltaTime = (static_cast<float>(dtSum) * INV_SAMPLES_COUNT) * INV_CYCLES_PER_SEC;

    // FPS Counter
    fpsCycleAccumulator += cyclesPassed;
    tempTimerIterationsCounter++;
    if (fpsCycleAccumulator >= EE_CYCLES_PER_SEC) {
        fpsCycleAccumulator -= EE_CYCLES_PER_SEC;
        timerIterationsCounter = tempTimerIterationsCounter;
        tempTimerIterationsCounter = 0;

    }

    // --- Accumulate ---
    physicsAcc += cyclesPassed;
    renderAcc += cyclesPassed;

    // Pre-calculate lerp for rendering interpolation
    stateLerp = static_cast<float>(physicsAcc) * INV_TARGET_PHYSICS;
    if (stateLerp > 1.0f) stateLerp = 1.0f;
}

bool Timer::updateFrame() {
    if (physicsAcc >= TARGET_PHYSICS_CYCLES) {
        physicsAcc -= TARGET_PHYSICS_CYCLES;
        
        // Measure interval since last updateFrame return
        u32 current = getCpuCycles();
        u32 diff = current - physicsBeginCpuCount;
        static constexpr float MS_FACTOR = 1000.0f / (float)EE_CYCLES_PER_SEC;
        physicsMs = (float)diff * MS_FACTOR;
        physicsBeginCpuCount = current;
        
        // Spiral of death prevention
        // If we are too far behind, clamp the accumulator
        if (physicsAcc > TARGET_PHYSICS_CYCLES * MAX_FRAME_SKIP) {
            physicsAcc = 0;
        }

        // Recalculate stateLerp with the drained accumulator so renderFrame()
        // uses the correct interpolation factor for this physics step
        stateLerp = static_cast<float>(physicsAcc) * INV_TARGET_PHYSICS;
        if (stateLerp > 1.0f) stateLerp = 1.0f;

        return true;
    }
    return false;
}

bool Timer::renderFrame() {
  bool result = false;

  if (g_settings.fps_mode == FpsMode::VSync) {
   if (*GS_REG_CSR & 8) {           // VSync ready?
        *GS_REG_CSR = 8;             // Clear flag (write-1-to-clear)
        result = true;
        renderAcc = 0;
    }
  } else {
    // When VSync is disabled, we rely on the accumulator
    if (renderAcc >= targetRenderCycles) {
        renderAcc -= targetRenderCycles;
        result = true;
    }
    
    // Spiral prevention for Render (only needed when unlimited/accumulating)
    if (renderAcc > targetRenderCycles * MAX_FRAME_SKIP) {
        renderAcc = targetRenderCycles;
    }
  }

  if (result) {
    u32 current = getCpuCycles();
    u32 diff = current - renderBeginCpuCount;
    static constexpr float MS_FACTOR = 1000.0f / (float)EE_CYCLES_PER_SEC;
    renderMs = (float)diff * MS_FACTOR;
    renderBeginCpuCount = current;
  }
  return result;
}

void Timer::setFpsMode(FpsMode mode) {
  switch (mode) {
    case FpsMode::FPS_30:
      targetRenderCycles = RENDER_CYCLES_30FPS;
      break;
    case FpsMode::FPS_60:
    default:
      targetRenderCycles = RENDER_CYCLES_60FPS;
      break;
  }
  // Reset accumulator so we don't service a huge backlog after switching
  renderAcc = targetRenderCycles / 2;
}


} // namespace TyraCraft