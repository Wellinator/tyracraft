#pragma once

#include "managers/world_generation/feature.hpp"

class DeadBushFeature : public Feature {
public:
    DeadBushFeature() : Feature(false) {}
    ~DeadBushFeature() override = default;

    bool place(Level* level, int x, int y, int z) override;
};
