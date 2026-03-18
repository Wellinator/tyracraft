#pragma once

#include "managers/world_generation/feature.hpp"

class OreFeature : public Feature {
private:
    uint8_t oreBlockType;
    uint8_t targetBlockType;
    int count;

public:
    OreFeature(uint8_t t_oreBlockType, int t_count, uint8_t t_targetBlockType = (uint8_t)Blocks::STONE_BLOCK)
        : Feature(false), oreBlockType(t_oreBlockType), targetBlockType(t_targetBlockType), count(t_count) {}
    ~OreFeature() override = default;

    /**
     * @brief Generates an ore vein starting at the given coordinates.
     */
    bool place(Level* level, int x, int y, int z) override;
};
