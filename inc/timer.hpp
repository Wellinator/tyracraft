#pragma once

#include "tyra"
#include "singleton.hpp"
#include <tamtypes.h>
#include "managers/settings_manager.hpp"

namespace TyraCraft {

/**
 * @brief High precision timer using EE Cycle Counter (COP0 Count)
 *
 * PS2 EE CPU Frequency: ~294.912 MHz
 * COP0 Count increments at Bus Clock (1/2 CPU Clock approx? No, actually:
 * The R5900 Count register increments at half the CPU frequency.
 * F_CPU = 294912000 Hz
 * F_COUNT = 147456000 Hz
 */
class Timer : public Singleton<Timer> {
 public:
  void init();
  void update();
  
  /**
   * @brief Sets the FPS mode, updating the render cycle target.
   * Resets the render accumulator to avoid stale values.
   */
  void setFpsMode(FpsMode mode);

  /**
   * @brief Checks if it's time to render a frame.
   * Handles VSync waiting if enabled.
   */
  bool renderFrame();

  /**
   * @brief Checks if it's time to run a physics/logic update.
   * Consumes accumulated time in fixed steps.
   */
  bool updateFrame();

  /**
   * @brief Helper to track elapsed time locally (in seconds).
   * Useful for events that happen after N seconds.
   */
  struct ElapsedTimer {
      float limit;
      float current;

      ElapsedTimer(float limitInSec = 0.0f) : limit(limitInSec), current(0.0f) {}

      // Returns true if the limit has been reached, and resets the timer
      inline bool update(float dt) {
          current += dt;
          if (current >= limit) {
              current = 0.0f; // loop
              return true;
          }
          return false;
      }

      inline void reset() { current = 0.0f; }
  };

  // Const getters (computed from internal integer state)
  inline u32 getUpdateTime() const { return timerIterationsCounter; }
  
  // Returns time in seconds (float) for compatibility
  inline float getDeltaTime() const { return realDeltaTime; }
  inline float getFixedDeltaTime() const { return fixedDeltaTimeF; }
  
  // Returns average delta time in seconds
  inline float getDeltaTimeAvg() const { return avgDeltaTime; }
  
  // Debug/Profile Stats (in milliseconds)
  inline float getRenderMs() const { return renderMs; }
  inline float getPhysicsUpdateMs() const { return physicsMs; }

  // Video mode info (for debugging vsync/field behavior)
  inline bool getIsInterlaced() const { return isInterlaced; }
  inline u8 getFieldCounter() const { return fieldCounter; }
  inline u8 getLastFieldBit() const { return lastFieldBit; }
    inline u32 getRenderFrameCalls() const { return renderFrameCalls; }
    inline u32 getFieldToggleDetected() const { return fieldToggleDetected; }

  // Interpolation factor for rendering [0.0, 1.0]
  static float stateLerp;

 private:
  // --- Constants (EE Cycle Counts) ---
  // 147,456,000 cycles per second
  static constexpr u32 EE_CYCLES_PER_SEC = 147456000;
  
  // Helpers to convert seconds/ms to cycles
  static constexpr u32 MS_TO_CYCLES(float ms) {
    return static_cast<u32>(ms * (static_cast<float>(EE_CYCLES_PER_SEC) / 1000.0f));
  }
  
  static constexpr u32 SEC_TO_CYCLES(float sec) {
      return static_cast<u32>(sec * static_cast<float>(EE_CYCLES_PER_SEC));
  }

  // FPS target cycle counts
  static constexpr u32 RENDER_CYCLES_60FPS = EE_CYCLES_PER_SEC / 60; // 2,457,600
  static constexpr u32 RENDER_CYCLES_30FPS = EE_CYCLES_PER_SEC / 30; // 4,915,200

  // Target cycles for 20 TPS update (Physics) -> ~50ms (Minecraft tick rate)
  static constexpr u32 TARGET_PHYSICS_CYCLES = EE_CYCLES_PER_SEC / 20;

  // Inverse of physics target (precomputed for stateLerp, shared by update() and updateFrame())
  static constexpr float INV_TARGET_PHYSICS = 1.0f / static_cast<float>(TARGET_PHYSICS_CYCLES);

  // Max skip to prevent spiral of death
  static constexpr u8 MAX_FRAME_SKIP = 2;

  // --- State ---
  
  u32 lastCpuCount = 0;       // Snapshot of COUNT register
  u32 renderAcc = 0;          // Accumulated cycles for rendering
  u32 physicsAcc = 0;         // Accumulated cycles for physics
  u32 targetRenderCycles = RENDER_CYCLES_60FPS; // Current FPS target
  
  // Timing data for getters
  float realDeltaTime = 0.0f;
  float avgDeltaTime = 0.0f;
  float fixedDeltaTimeF = 1.0f / 20.0f; // 0.05s — Minecraft tick rate

  // Profiling data
  u32 renderBeginCpuCount = 0;
  float renderMs = 0.0f;
  
  u32 physicsBeginCpuCount = 0;
  float physicsMs = 0.0f;

  // FPS Counter helpers
  u32 fpsCycleAccumulator = 0; // Accumulates cycles to count 1 second
  u32 timerIterationsCounter = 0;
  u32 tempTimerIterationsCounter = 0;

  // Average Delta Time helpers
  u32 dtSamples[10] = {0};
  u8 dtIndex = 0;
  u32 dtSum = 0; // Sum of cycles in buffer

  // VSync field tracking (for interlaced mode detection)
  u8 lastFieldBit = 0;      // Previous state of GS_REG_CSR bit 13 (FIELD)
  u8 fieldCounter = 0;      // Counts fields in interlaced mode (2 fields = 1 frame)
  bool isInterlaced = true; // Video mode flag (detected at init, assume interlaced by default)
  
    // Diagnostic counters
    u32 renderFrameCalls = 0;     // Total renderFrame() calls
    u32 fieldToggleDetected = 0;  // Field bit transitions detected
  

  // --- Internal Helpers ---
  inline u32 getCpuCycles() const {
      u32 count;
      asm volatile ("mfc0 %0, $9" : "=r" (count));
      return count;
  }
};

}  // namespace TyraCraft
