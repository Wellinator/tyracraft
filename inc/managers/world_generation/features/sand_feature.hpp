#pragma once

#include "managers/world_generation/feature.hpp"

namespace TyraCraft {

class SandFeature : public Feature {
 private:
  uint8_t tile;
  int radius;

 public:
  SandFeature(int radius, uint8_t tile);
  ~SandFeature() override = default;

  bool place(Level* level, int x, int y, int z) override;
};

}  // namespace TyraCraft
