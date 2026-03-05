#pragma once

#include <tamtypes.h>
#include "constants.hpp"

namespace TyraCraft {

/**
 * @brief Lightweight metadata about a block edit (placement/removal).
 *
 * Captures all information needed to optimize chunk updates:
 * - Which block was edited and why (old vs. new type)
 * - Light values before/after (determines propagation distance)
 * - Whether geometry or light is affected (determines which chunks rebuild)
 *
 * Used to avoid duplicates and unnecessary chunk rebuilds during block edits.
 * Size: ~16 bytes (Vec4 + 4×u8 + alignment).
 */
struct EditContext {
  // --- Edit location & types ---
  Vec4 blockPos;                    // Position in world (x, y, z, padding)
  Blocks oldBlockType = Blocks::AIR_BLOCK;  // Type before edit
  Blocks newBlockType = Blocks::AIR_BLOCK;  // Type after edit

  // --- Light values (0-15, or 0 if not light-emitting) ---
  u8 oldLightValue = 0;     // Block light level of block removed (0-15, usually 0)
  u8 newLightValue = 0;     // Block light level of block placed (0-15, usually 0)

  // --- Impact flags ---
  bool isLightEmitter = false;      // true if newBlockType emits light (lightValue > 0)
  bool affectsGeometry = false;     // true if old→new changes collision/transparency
  u8 _padding = 0;                  // Alignment padding

  // --- Helpers ---

  /**
   * @brief Returns true if this edit prevents light propagation.
   * Opaque blocks block light (and sunlight decreases).
   */
  inline bool isLightBlocking() const {
    // Opaque blocks typically have no lightValue and different transparency
    return !isLightEmitter && affectsGeometry;
  }

  /**
   * @brief Compute expected propagation distance in chunks.
   * Light spreads up to 15 blocks (diagonal max ≈ 15√3 ≈ 26 blocks).
   * Each chunk is 16 blocks, so: 15 / 16 ≈ 1 chunk, with some overspill.
   * Rounded up: ~2.0 chunks is a safe over-estimate.
   */
  inline float getLightPropagationRadiusInChunks() const {
    // Max light value is 15; each chunk is 16 blocks wide
    // So radius = max(oldLightValue, newLightValue) / 16 ≈ ~1.0 chunk
    // But to be safe and account for corner propagation, use 2.0
    u8 maxLightVal = (oldLightValue > newLightValue) ? oldLightValue : newLightValue;
    return (maxLightVal > 0) ? 2.0f : 1.0f;  // 2.0 for emitter/remover, 1.0 otherwise
  }
};

}  // namespace TyraCraft
