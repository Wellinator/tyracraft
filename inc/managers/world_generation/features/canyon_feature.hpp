#pragma once

#include "managers/world_generation/feature.hpp"

class CanyonFeature : public Feature {
 public:
  CanyonFeature() : Feature(false) {}
  ~CanyonFeature() override = default;

  /**
   * @brief Generates a canyon (ravine) starting at the given coordinates.
   */
  bool place(Level* level, int x, int y, int z) override;

 private:
  void fillOblateSpheroid(Level* pLevel, int center_x, int center_y,
                          int center_z, float radX, float radY, float radZ,
                          uint8_t blk);
};
