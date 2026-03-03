#include "managers/draw_distance_controller.hpp"
#include <algorithm>

void DrawDistanceController::init(DrawDistanceMode mode) {
  currentMode = mode;
}

void DrawDistanceController::setMode(DrawDistanceMode mode) {
  currentMode = mode;
}

DrawDistanceMode DrawDistanceController::getMode() const {
  return currentMode;
}

bool DrawDistanceController::canLoadMoreChunks() const {
  return getUsedMemoryMb() < getMemoryThresholdMb();
}

size_t DrawDistanceController::getUsedMemoryMb() const {
  return (get_used_memory() >> 20);
}

size_t DrawDistanceController::getMemoryThresholdMb() const {
  return MAX_SAFE_MEMORY_ALLOCATION - DRAW_DISTANCE_SAFETY_MARGIN_MB;
}

u8 DrawDistanceController::getEffectiveRadius() const {
  u8 modeCap = getDrawDistanceCap(currentMode);
  return std::min(modeCap, static_cast<u8>(MAX_DRAW_DISTANCE));
}

float DrawDistanceController::getDirectionalRatio(const Vec4& directionToChunk,
                                                  const Vec4& cameraForward) const {
  // Dot product gives cos(angle) in [-1, 1]
  // -1 = behind, 0 = perpendicular (sides), 1 = forward
  const float dot = directionToChunk.x * cameraForward.x +
                    directionToChunk.z * cameraForward.z;

  // Map cos to [0, 1]: t = (dot + 1) / 2
  float t = (dot + 1.0f) * 0.5f;
  t = std::max(0.0f, std::min(1.0f, t));

  // Smooth interpolation constrained by three design anchors:
  // t=0.0 -> BACKWARD ratio, t=0.5 -> SIDE ratio, t=1.0 -> FRONT ratio (1.0)
  const float back = DRAW_DISTANCE_BACKWARD_RATIO;
  const float side = DRAW_DISTANCE_SIDE_RATIO;
  const float front = 1.0f;

  const float b = 4.0f * side - 3.0f * back - front;
  const float a = front - back - b;
  float ratio = ((a * t) + b) * t + back;

  // Keep ratio safe even if tuning constants are changed.
  ratio = std::max(0.0f, std::min(front, ratio));
  return ratio;
}

float DrawDistanceController::getEffectiveLoadDistanceSq(
    const Vec4& directionToChunk, const Vec4& cameraForward) const {
  float ratio = getDirectionalRatio(directionToChunk, cameraForward);
  float baseRadius = static_cast<float>(getEffectiveRadius());
  // Scale from chunk-count to block-grid units (block-grid is CHUNK_SIZE times larger)
  float effectiveRadius = baseRadius * ratio * static_cast<float>(CHUNK_SIZE);

  return effectiveRadius * effectiveRadius;
}

bool DrawDistanceController::isInLoadableArea(const Vec4& chunkCenter,
                                              const Vec4& playerPos,
                                              const Vec4& cameraForward) const {
  // Compute direction from player to chunk
  Vec4 diff = chunkCenter;
  diff.x -= playerPos.x;
  diff.y -= playerPos.y;
  diff.z -= playerPos.z;

  // Use XZ-plane direction for ratio calculation
  Vec4 directionXZ;
  directionXZ.x = diff.x;
  directionXZ.z = diff.z;
  directionXZ.y = 0.0f;

  // Normalize direction
  float lenXZ = std::sqrt(directionXZ.x * directionXZ.x +
                          directionXZ.z * directionXZ.z);
  if (lenXZ < 0.001f) {
    // Player is essentially at chunk center, allow load
    return true;
  }
  directionXZ.x /= lenXZ;
  directionXZ.z /= lenXZ;

  // Get effective load distance threshold
  float effectiveLoadDistSq = getEffectiveLoadDistanceSq(directionXZ, cameraForward);

  // Load threshold: use directional distance for comparison
  // (we care about horizontal distance, not vertical)
  float horizontalDistSq = diff.x * diff.x + diff.z * diff.z;

  return horizontalDistSq <= effectiveLoadDistSq;
}

bool DrawDistanceController::isInUnloadableArea(const Vec4& chunkCenter,
                                                const Vec4& playerPos,
                                                const Vec4& cameraForward) const {
  // Compute direction from player to chunk
  Vec4 diff = chunkCenter;
  diff.x -= playerPos.x;
  diff.y -= playerPos.y;
  diff.z -= playerPos.z;

  // Use XZ-plane direction for ratio calculation
  Vec4 directionXZ;
  directionXZ.x = diff.x;
  directionXZ.z = diff.z;
  directionXZ.y = 0.0f;

  // Normalize direction
  float lenXZ = std::sqrt(directionXZ.x * directionXZ.x +
                          directionXZ.z * directionXZ.z);
  if (lenXZ < 0.001f) {
    // Player is essentially at chunk center, keep it
    return false;
  }
  directionXZ.x /= lenXZ;
  directionXZ.z /= lenXZ;

  // Get effective load distance threshold
  float effectiveLoadDistSq = getEffectiveLoadDistanceSq(directionXZ, cameraForward);

  // Unload threshold: effectiveLoadDistSq + hysteresis margin (scaled to block-grid units)
  float unloadMargin = static_cast<float>(DRAW_DISTANCE_UNLOAD_MARGIN * CHUNK_SIZE);
  float unloadThresholdSq = 
      (std::sqrt(effectiveLoadDistSq) + unloadMargin) *
      (std::sqrt(effectiveLoadDistSq) + unloadMargin);

  // Horizontal distance squared
  float horizontalDistSq = diff.x * diff.x + diff.z * diff.z;

  return horizontalDistSq > unloadThresholdSq;
}

float DrawDistanceController::getDirectionalDistanceSq(const Vec4& chunkCenter,
                                                       const Vec4& playerPos,
                                                       const Vec4& cameraForward) const {
  // Compute direction from player to chunk
  Vec4 diff = chunkCenter;
  diff.x -= playerPos.x;
  diff.y -= playerPos.y;
  diff.z -= playerPos.z;

  // Use XZ-plane direction for ratio calculation
  Vec4 directionXZ;
  directionXZ.x = diff.x;
  directionXZ.z = diff.z;
  directionXZ.y = 0.0f;

  // Normalize direction
  float lenXZ = std::sqrt(directionXZ.x * directionXZ.x +
                          directionXZ.z * directionXZ.z);
  if (lenXZ < 0.001f) {
    return 0.0f;
  }
  directionXZ.x /= lenXZ;
  directionXZ.z /= lenXZ;

  // Get the actual horizontal distance
  float horizontalDistSq = diff.x * diff.x + diff.z * diff.z;

  // Get directional ratio
  float ratio = getDirectionalRatio(directionXZ, cameraForward);

  // Apply directional weighting to the actual distance
  // Chunks in preferred directions (forward) are weighted as closer
  // This allows proper sorting: chunks in front get higher priority
  
  // A chunk at the same actual distance should sort differently based on direction:
  // - Forward (ratio=1.0): full distance weight
  // - Side (ratio=0.7): 0.7x distance weight (appears "closer" in priority)
  // - Back (ratio=0.4): 0.4x distance weight (appears "closer" in priority)
  
  // We want SMALLER values for higher priority, so we divide by ratio
  if (ratio < 0.001f) {
    return horizontalDistSq;
  }

  // Return directional weighted squared distance
  // Higher ratio (forward) = smaller divisor = larger result = lower priority (correct!)
  // Wait, that's backwards. We want forward to have HIGHER priority (smaller value)
  // So we should MULTIPLY by ratio, not divide
  // Forward (ratio=1.0): distance * 1.0 = full distance
  // Side (ratio=0.7): distance * 0.7 = appears 30% closer
  // Back (ratio=0.4): distance * 0.4 = appears 60% closer
  // But we want FORWARD to load first, not back!
  // So we need to INVERT the ratio weighting
  
  // Correct formula: divide by ratio to make forward chunks have smaller sort key
  // Forward (ratio=1.0): dist² / 1.0 = dist² (baseline)
  // Side (ratio=0.7): dist² / 0.7 = dist² * 1.43 (lower priority, loads later)
  // Back (ratio=0.4): dist² / 0.4 = dist² * 2.5 (lowest priority, loads last)
  return horizontalDistSq / ratio;
}
