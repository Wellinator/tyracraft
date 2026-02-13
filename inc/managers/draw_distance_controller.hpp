#pragma once

#include <tamtypes.h>
#include <math/vec4.hpp>
#include <cmath>
#include "constants.hpp"
#include "memory-monitor/memory_monitor.hpp"

using Tyra::Vec4;

class DrawDistanceController {
 public:
  DrawDistanceController() = default;
  ~DrawDistanceController() = default;

  void init(const u8 minForwardDistance) {
    minimumForwardDistance = minForwardDistance;
    currentForwardDistance = minForwardDistance;
    smoothedForward = Vec4(0.0f, 0.0f, -1.0f);
    hasSmoothedForward = false;
  }

  void setMinimumForwardDistance(const u8 minForwardDistance) {
    minimumForwardDistance = minForwardDistance;
    if (currentForwardDistance < minimumForwardDistance) {
      currentForwardDistance = minimumForwardDistance;
    }
  }

  void update(const Vec4& camForward) {
    updateSmoothedForward(camForward);
    updateDistanceByMemory();
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

  inline bool canLoadMoreChunks() const {
    return getUsedMemoryMb() < getMemoryThresholdMb();
  }

  inline const Vec4& getSmoothedForward() const { return smoothedForward; }

 private:
  u8 minimumForwardDistance = MIN_DRAW_DISTANCE;
  u8 currentForwardDistance = MIN_DRAW_DISTANCE;
  Vec4 smoothedForward = Vec4(0.0f, 0.0f, -1.0f);
  bool hasSmoothedForward = false;

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

  void updateDistanceByMemory() {
    const size_t usedMb = getUsedMemoryMb();
    const size_t thresholdMb = getMemoryThresholdMb();

    if (usedMb < thresholdMb) {
      if (currentForwardDistance < MAX_DRAW_DISTANCE) {
        currentForwardDistance++;
      }
    } else {
      if (currentForwardDistance > minimumForwardDistance) {
        currentForwardDistance--;
      }
    }
  }
};
