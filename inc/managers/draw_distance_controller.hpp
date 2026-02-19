#pragma once

#include <tamtypes.h>
#include <math/vec4.hpp>
#include <cmath>
#include "constants.hpp"
#include "memory-monitor/memory_monitor.hpp"
#include "timer.hpp"

using Tyra::Vec4;

class DrawDistanceController {
 public:
  DrawDistanceController() = default;
  ~DrawDistanceController() = default;

  void init(DrawDistanceMode mode) {
    currentMode = mode;
    minimumForwardDistance = MIN_DRAW_DISTANCE;
    currentForwardDistance = MIN_DRAW_DISTANCE;
    smoothedForward = Vec4(0.0f, 0.0f, -1.0f);
    hasSmoothedForward = false;
  }

  void setMode(DrawDistanceMode mode) {
    currentMode = mode;
    const u8 cap = getDrawDistanceCap(mode);
    if (currentForwardDistance > cap) {
      currentForwardDistance = cap;
    }
  }

  inline DrawDistanceMode getMode() const { return currentMode; }

  void update(const Vec4& camForward) {
    updateSmoothedForward(camForward);
    updateDistance();
  }

  inline u8 getForwardDistance() const { return currentForwardDistance; }

  inline u8 getBackwardDistance() const {
    const float back = static_cast<float>(currentForwardDistance) *
                       DRAW_DISTANCE_BACKWARD_RATIO;
    return static_cast<u8>(back < 1.0f ? 1 : back);
  }

  inline float getSideDistance() const {
    return static_cast<float>(currentForwardDistance) * DRAW_DISTANCE_SIDE_RATIO;
  }

  inline u8 getUnloadDistance() const {
    return currentForwardDistance + DRAW_DISTANCE_UNLOAD_MARGIN;
  }

  inline bool canLoadMoreChunks() const {
    return getUsedMemoryMb() < getMemoryThresholdMb();
  }

  inline const Vec4& getSmoothedForward() const { return smoothedForward; }

 private:
  DrawDistanceMode currentMode = DrawDistanceMode::Auto;
  u8 minimumForwardDistance = MIN_DRAW_DISTANCE;
  u8 currentForwardDistance = MIN_DRAW_DISTANCE;
  Vec4 smoothedForward = Vec4(0.0f, 0.0f, -1.0f);
  bool hasSmoothedForward = false;
  u16 cooldownTicksRemaining = 0;

  inline size_t getUsedMemoryMb() const { return (get_used_memory() >> 20); }

  inline size_t getMemoryThresholdMb() const {
    return MAX_SAFE_MEMORY_ALLOCATION - DRAW_DISTANCE_SAFETY_MARGIN_MB;
  }

  void updateSmoothedForward(const Vec4& camForward) {
    Vec4 target = camForward;
    target.y = 0.0f;
    const float lenSq = target.x * target.x + target.z * target.z;
    if (lenSq < 0.0001f) {
      target = Vec4(0.0f, 0.0f, -1.0f);
    } else {
      const float invLen = 1.0f / sqrtf(lenSq);
      target.x *= invLen;
      target.z *= invLen;
    }

    if (!hasSmoothedForward) {
      smoothedForward = target;
      hasSmoothedForward = true;
      return;
    }

    const float blend = 0.05f;
    smoothedForward.x = smoothedForward.x + (target.x - smoothedForward.x) * blend;
    smoothedForward.z = smoothedForward.z + (target.z - smoothedForward.z) * blend;
    smoothedForward.y = 0.0f;
  }

  void updateDistance() {
    if (cooldownTicksRemaining > 0) {
      cooldownTicksRemaining--;
      return;
    }

    const size_t usedMb = getUsedMemoryMb();
    const size_t thresholdMb = getMemoryThresholdMb();
    const size_t growThreshold = (thresholdMb >= 2) ? (thresholdMb - 2) : thresholdMb;
    const size_t shrinkThreshold = thresholdMb;

    const u32 fps = TyraCraft::Timer::getInstance()->getUpdateTime();
    const u8 maxDistance = getDrawDistanceCap(currentMode);

    bool changed = false;
    bool shrank = false;

    // Shrink: high memory OR low FPS
    if (usedMb >= shrinkThreshold ||
        fps < DRAW_DISTANCE_FPS_SHRINK_THRESHOLD) {
      if (currentForwardDistance > MIN_DRAW_DISTANCE) {
        currentForwardDistance--;
        changed = true;
        shrank = true;
      }
    }
    // Grow: memory OK AND FPS good AND below mode cap
    else if (usedMb < growThreshold &&
             fps > DRAW_DISTANCE_FPS_GROW_THRESHOLD) {
      if (currentForwardDistance < maxDistance) {
        currentForwardDistance++;
        changed = true;
      }
    }

    if (changed) {
      cooldownTicksRemaining = shrank ? DRAW_DISTANCE_SHRINK_COOLDOWN
                                      : DRAW_DISTANCE_GROW_COOLDOWN;
    }
  }
};
