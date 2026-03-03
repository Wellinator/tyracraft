#pragma once

#include <tamtypes.h>
#include <math/vec4.hpp>
#include <cmath>
#include "constants.hpp"
#include "memory-monitor/memory_monitor.hpp"
#include "timer.hpp"

using Tyra::Vec4;

/**
 * DrawDistanceController — Central authority for directional chunk loading.
 *
 * Implements an elliptical loading area around the player using directional
 * ratios to modulate load distance:
 *   - Forward (facing): 100% of base radius
 *   - Sides: 70% of base radius
 *   - Behind: 40% of base radius
 *
 * Hysteresis prevents load/unload oscillation:
 *   - Load threshold: effectiveRadius
 *   - Unload threshold: effectiveRadius + DRAW_DISTANCE_UNLOAD_MARGIN
 *   - Chunks in the dead zone (between thresholds) are kept as-is.
 */
class DrawDistanceController {
 public:
  DrawDistanceController() = default;
  ~DrawDistanceController() = default;

  // ---- Lifecycle ----
  void init(DrawDistanceMode mode);
  void setMode(DrawDistanceMode mode);
  DrawDistanceMode getMode() const;

  // ---- Memory check ----
  /**
   * Check if RAM budget allows more chunk loading.
   * @return true if used memory < threshold
   */
  bool canLoadMoreChunks() const;

  // ---- Directional area queries ----
  /**
   * Check if chunk center is within the loadable area (elliptical region).
   * Uses hysteresis: load threshold = effectiveRadius.
   *
   * @param chunkCenter Center position of the chunk (in world coordinates)
   * @param playerPos Current player position
   * @param cameraForward Camera facing direction (should be normalized)
   * @return true if chunk distance <= loadable threshold
   */
  bool isInLoadableArea(const Vec4& chunkCenter, const Vec4& playerPos,
                        const Vec4& cameraForward) const;

  /**
   * Check if chunk center is outside the loadable area (too far to keep).
   * Uses hysteresis: unload threshold = effectiveRadius + DRAW_DISTANCE_UNLOAD_MARGIN.
   *
   * @param chunkCenter Center position of the chunk (in world coordinates)
   * @param playerPos Current player position
   * @param cameraForward Camera facing direction (should be normalized)
   * @return true if chunk distance > unload threshold
   */
  bool isInUnloadableArea(const Vec4& chunkCenter, const Vec4& playerPos,
                          const Vec4& cameraForward) const;

  /**
   * Compute directional distance squared for sorting (nearest chunks first).
   * This accounts for the elliptical modulation (forward/side/back ratios).
   *
   * @param chunkCenter Center position of the chunk (in world coordinates)
   * @param playerPos Current player position
   * @param cameraForward Camera facing direction (should be normalized)
   * @return Distance squared in the directional space
   */
  float getDirectionalDistanceSq(const Vec4& chunkCenter, const Vec4& playerPos,
                                 const Vec4& cameraForward) const;

  /**
   * Get the effective load radius in chunks, accounting for mode/RAM limits.
   * This is the base radius before directional modulation.
   *
   * @return Effective radius in chunks
   */
  u8 getEffectiveRadius() const;

 private:
  DrawDistanceMode currentMode = DrawDistanceMode::Auto;

  // ---- Memory helpers ----
  size_t getUsedMemoryMb() const;
  size_t getMemoryThresholdMb() const;

  // ---- Directional math helpers ----
  /**
   * Compute the directional ratio [0.4, 0.7, 1.0] based on angle.
   * Maps the dot product between direction-to-chunk and camera forward
   * to a ratio that scales the base radius.
   *
   * Interpolation:
   *   t = (dot + 1) / 2  →  [0, 1]  (0=behind, 0.5=side, 1=forward)
   *   ratio = BACK_RATIO + t * (1.0 - BACK_RATIO)
   *
   * @param directionToChunk Normalized direction from player to chunk (XZ plane)
   * @param cameraForward Normalized camera forward (XZ plane)
   * @return Ratio in [BACK_RATIO, 1.0]
   */
  float getDirectionalRatio(const Vec4& directionToChunk,
                            const Vec4& cameraForward) const;

  /**
   * Compute effective load distance considering directional ratio.
   * @return Effective radius (base radius × directional ratio)
   */
  float getEffectiveLoadDistanceSq(const Vec4& directionToChunk,
                                   const Vec4& cameraForward) const;
};
