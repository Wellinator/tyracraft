#pragma once

#include <stdint.h>

/**
 * @brief Lightweight BFS node used in light and liquid propagation.
 */
struct BfsNode {
  uint16_t x, y, z;
  uint16_t val;

  BfsNode(uint16_t lx, uint16_t ly, uint16_t lz, uint16_t l)
      : x(lx), y(ly), z(lz), val(l) {}
};
