#pragma once

#include "managers/world_generation/feature.hpp"
#include "managers/world_generation/features/tree_feature.hpp"

class MazeRoomFeature : public Feature {
private:
    TreeFeature oakTreeFeature;
    TreeFeature birchTreeFeature;

public:
    MazeRoomFeature();
    ~MazeRoomFeature() override = default;

    bool place(Level* level, int x, int y, int z) override;
};
