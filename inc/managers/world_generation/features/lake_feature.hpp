#pragma once

#include "managers/world_generation/feature.hpp"

class LakeFeature : public Feature {
private:
    uint8_t liquidBlockType;

public:
    LakeFeature(uint8_t t_liquidBlockType)
        : Feature(false), liquidBlockType(t_liquidBlockType) {}
    ~LakeFeature() override = default;

    /**
     * @brief Generates an underground lake or source.
     */
    bool place(Level* level, int x, int y, int z) override;
};
