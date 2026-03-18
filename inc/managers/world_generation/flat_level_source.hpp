#pragma once

#include "managers/world_generation/chunk_source.hpp"
#include <constants.hpp>

namespace TyraCraft {

class FlatLevelSource : public ChunkSource {
public:
    FlatLevelSource();
    ~FlatLevelSource() override;

    void generateChunk(Level* level, int chunkX, int chunkZ) override;
    void carve(Level* level, int chunkX, int chunkZ) override;
    void postProcess(Level* level, int chunkX, int chunkZ) override;
};

}  // namespace TyraCraft
