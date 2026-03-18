#pragma once

#include "managers/world_generation/feature.hpp"

class CactusFeature : public Feature {
public:
    CactusFeature() : Feature(false) {}
    ~CactusFeature() override = default;

    bool place(Level* level, int x, int y, int z) override;
};
