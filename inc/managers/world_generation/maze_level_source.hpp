#pragma once

#include "managers/world_generation/chunk_source.hpp"
#include "managers/world_generation/features/maze_room_feature.hpp"
#include "3libs/mazegen/mazegen.hpp"
#include <constants.hpp>

namespace TyraCraft {

class MazeLevelSource : public ChunkSource {
private:
    uint32_t seed;
    u8 width;
    u8 height;
    mazegen::Config cfg;
    mazegen::Generator gen;
    mazegen::PointSet constraints;
    MazeRoomFeature roomFeature;

public:
    MazeLevelSource(uint32_t seed);
    ~MazeLevelSource() override = default;

    void generateChunk(Level* level, int chunkX, int chunkZ) override;
    void postProcess(Level* level, int chunkX, int chunkZ) override;

private:
    void initMazegen();
};

}  // namespace TyraCraft
