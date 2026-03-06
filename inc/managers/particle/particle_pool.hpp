#pragma once

#include <stddef.h>
#include "constants.hpp"

/**
 * ParticlePool<T, N>
 *
 * Fixed-size object pool for particle instances.
 * - Stores N objects in a contiguous array (no heap per-particle).
 * - acquire() is O(1) via LIFO free-list.
 * - release(T*) is O(1) — marks slot available.
 * - Iterating active particles via activeCount + activeIndices.
 *
 * Constraints:
 * - T must be trivially constructible (pool placement-initialises via reset()).
 * - T must have an `expired` field (u8).
 */
template <typename T, u16 N>
class ParticlePool {
 public:
  static constexpr u16 CAPACITY = N;

  ParticlePool() {
    // All slots start free; free-list is a simple stack (top of stack = freeTop)
    for (u16 i = 0; i < N; i++) {
      freeSlots[i] = i;
      active[i] = false;
    }
    freeTop = N;  // points one past the last valid free-list entry
    _activeCount = 0;
  }

  ~ParticlePool() = default;

  // Returns a pointer to a free slot, or nullptr if pool is full.
  // The caller is responsible for constructing the object via placement-new
  // or calling a reset/init method on the returned pointer.
  T* acquire() {
    if (freeTop == 0) return nullptr;
    const u16 idx = freeSlots[--freeTop];
    active[idx] = true;
    _activeCount++;
    return &pool[idx];
  }

  // Mark a slot as available again.
  void release(T* ptr) {
    const u16 idx = static_cast<u16>(ptr - pool);
    if (idx >= N || !active[idx]) return;
    active[idx] = false;
    freeSlots[freeTop++] = idx;
    _activeCount--;
  }

  // Release all active slots whose expired flag is true.
  // Calls T destructor equivalent via ptr->~T() only if T has non-trivial dtor.
  void flushExpired() {
    for (u16 i = 0; i < N; i++) {
      if (active[i] && pool[i].expired) {
        active[i] = false;
        freeSlots[freeTop++] = i;
        _activeCount--;
      }
    }
  }

  // Release all active slots unconditionally.
  void reset() {
    for (u16 i = 0; i < N; i++) {
      active[i] = false;
    }
    for (u16 i = 0; i < N; i++) {
      freeSlots[i] = i;
    }
    freeTop = N;
    _activeCount = 0;
  }

  // Iterate over all active objects.
  // Usage: for (u16 i = 0; i < pool.capacity(); i++) { if (pool.isActive(i)) ... }
  inline bool isActive(u16 idx) const { return active[idx]; }
  inline T*   get(u16 idx)            { return &pool[idx]; }
  inline u16  capacity()       const  { return N; }
  inline u16  activeCount()    const  { return _activeCount; }
  inline bool full()           const  { return freeTop == 0; }

 private:
  T    pool[N];
  bool active[N];
  u16  freeSlots[N];
  u16  freeTop;
  u16  _activeCount;
};
