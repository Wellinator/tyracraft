#pragma once

#include "entities/level.hpp"

enum class TerrainType {
    Original,
    Island,
    Woods,
    Flat,
    Floating,
    Maze
};

class ChunkSource {
public:
    virtual ~ChunkSource() {}

    /**
     * @brief Generates the chunk at the given X and Z coordinates.
     * In TyraCraft, this populates the blocks directly into the Level's LevelMap.
     */
    virtual void generateChunk(Level* level, int chunkX, int chunkZ) = 0;

    /**
     * @brief Post-processes the chunk (decorations, populating features, etc.)
     */
    virtual void postProcess(Level* level, int chunkX, int chunkZ) = 0;
};
