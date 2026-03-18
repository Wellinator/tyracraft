#pragma once

#include "managers/world_generation/feature.hpp"

class HouseFeature : public Feature {
public:
    HouseFeature() : Feature(false) {}
    ~HouseFeature() override = default;

    /**
     * @brief Generates a house starting at the given coordinates.
     */
    bool place(Level* level, int x, int y, int z) override;
};
