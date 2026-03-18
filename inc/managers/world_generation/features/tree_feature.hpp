#pragma once

#include "managers/world_generation/feature.hpp"

class TreeFeature : public Feature {
private:
    uint8_t logBlockType;
    uint8_t leavesBlockType;

public:
    TreeFeature(uint8_t t_logBlockType, uint8_t t_leavesBlockType)
        : Feature(false), logBlockType(t_logBlockType), leavesBlockType(t_leavesBlockType) {}
    ~TreeFeature() override = default;

    /**
     * @brief Generates a tree starting at the given coordinates.
     */
    bool place(Level* level, int x, int y, int z) override;

public:
    bool isSpaceForTree(Level* pLevel, int x, int y, int z, int treeHeight);
    void growTree(Level* pLevel, int x, int y, int z, int treeHeight);
};
