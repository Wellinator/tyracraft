#pragma once

#include "managers/world_generation/feature.hpp"

class TallGrassFeature : public Feature {
private:
    uint8_t grassBlockType;

public:
    TallGrassFeature(uint8_t t_grassBlockType)
        : Feature(false), grassBlockType(t_grassBlockType) {}
    ~TallGrassFeature() override = default;

    /**
     * @brief Generates tall grass blocks across an area.
     */
    bool place(Level* level, int x, int y, int z) override;
};
