#pragma once

#include "managers/world_generation/feature.hpp"

enum class PlantType {
    Grass,
    Flower,
    Mushroom
};

class PlantFeature : public Feature {
private:
    PlantType type;
    int offset;

public:
    PlantFeature(PlantType t_type, int t_offset = 1)
        : Feature(false), type(t_type), offset(t_offset) {}
    ~PlantFeature() override = default;

    /**
     * @brief Generates plants (grass, flowers, mushrooms) around a patch.
     */
    bool place(Level* level, int x, int y, int z) override;
};
