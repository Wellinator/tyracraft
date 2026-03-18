#pragma once

#include "managers/world_generation/feature.hpp"

class ReedsFeature : public Feature {
private:
    uint8_t reedsBlockType;

public:
    ReedsFeature(uint8_t t_reedsBlockType)
        : Feature(false), reedsBlockType(t_reedsBlockType) {}
    ~ReedsFeature() override = default;

    /**
     * @brief Generates reeds (sugar canes) near water sources.
     */
    bool place(Level* level, int x, int y, int z) override;
};
