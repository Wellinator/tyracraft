#pragma once

#include "managers/world_generation/feature.hpp"

class CaveFeature : public Feature {
public:
    CaveFeature() : Feature(false) {}
    ~CaveFeature() override = default;

    /**
     * @brief Generates a cave segment starting at the given coordinates.
     */
    bool place(Level* level, int x, int y, int z) override;
};
