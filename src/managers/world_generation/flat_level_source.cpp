#include "managers/world_generation/flat_level_source.hpp"

namespace TyraCraft {

FlatLevelSource::FlatLevelSource() {}

FlatLevelSource::~FlatLevelSource() {}

void FlatLevelSource::generateChunk(Level* level, int chunkX, int chunkZ) {
    int startX = chunkX * CHUNK_SIZE;
    int startZ = chunkZ * CHUNK_SIZE;

    for (int x = startX; x < startX + CHUNK_SIZE; x++) {
        for (int z = startZ; z < startZ + CHUNK_SIZE; z++) {
            if (!level->BoundCheckMap(x, 0, z)) continue;

            for (int y = 0; y < level->map.height; y++) {
                if (y == 0) {
                    level->SetBlockInMap(x, y, z, static_cast<uint8_t>(Blocks::BEDROCK_BLOCK));
                } else if (y <= 2) {
                    level->SetBlockInMap(x, y, z, static_cast<uint8_t>(Blocks::DIRTY_BLOCK));
                } else if (y == 3) {
                    level->SetBlockInMap(x, y, z, static_cast<uint8_t>(Blocks::GRASS_BLOCK));
                } else {
                    level->SetBlockInMap(x, y, z, static_cast<uint8_t>(Blocks::AIR_BLOCK));
                }
            }
        }
    }
}

void FlatLevelSource::carve(Level* level, int chunkX, int chunkZ) {
    // Flat worlds don't have carving features
    (void)level;
    (void)chunkX;
    (void)chunkZ;
}

void FlatLevelSource::postProcess(Level* level, int chunkX, int chunkZ) {
    // Currently no post-processing for flat worlds in the original implementation
    // Besides what we might want to add later like villages
}

}  // namespace TyraCraft
