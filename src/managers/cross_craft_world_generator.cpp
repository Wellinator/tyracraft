#include <managers/cross_craft_world_generator.hpp>
#include <managers/world_generation/random_level_source.hpp>
#include <managers/world_generation/flat_level_source.hpp>
#include <managers/world_generation/maze_level_source.hpp>
#include <utils.hpp>

static int32_t worldgen_seed;

void CrossCraft_WorldGenerator_Init(int32_t seed) {
    worldgen_seed = seed;
}

void process_chunk_source(Level* pLevel, TerrainType terrain) {
    ChunkSource* generator = nullptr;

    if (terrain == TerrainType::Flat || terrain == TerrainType::Woods) {
        generator = new TyraCraft::FlatLevelSource();
    } else if (terrain == TerrainType::Maze) {
        generator = new TyraCraft::MazeLevelSource(worldgen_seed);
    } else {
        generator = new RandomLevelSource(worldgen_seed, terrain);
    }

    int numChunksX = pLevel->map.length / CHUNK_SIZE;
    int numChunksZ = pLevel->map.width / CHUNK_SIZE;

    TYRA_LOG("WorldGen: Generating Chunks...");
    for (int cx = 0; cx < numChunksX; cx++) {
        for (int cz = 0; cz < numChunksZ; cz++) {
            generator->generateChunk(pLevel, cx, cz);
        }
    }

    TYRA_LOG("WorldGen: Post Processing Chunks...");
    // Another pass for post-processing so that neighbors exist
    for (int cx = 0; cx < numChunksX; cx++) {
        for (int cz = 0; cz < numChunksZ; cz++) {
            generator->postProcess(pLevel, cx, cz);
        }
    }

    delete generator;
}

void CrossCraft_WorldGenerator_Generate_Original(Level* pLevel) {
    process_chunk_source(pLevel, TerrainType::Original);
}

void CrossCraft_WorldGenerator_Generate_Island(Level* pLevel) {
    process_chunk_source(pLevel, TerrainType::Island);
}

void CrossCraft_WorldGenerator_Generate_Floating(Level* pLevel) {
    TYRA_WARN("Floating generation uses fallback terrain type!");
    process_chunk_source(pLevel, TerrainType::Floating);
}

void CrossCraft_WorldGenerator_Generate_Woods(Level* pLevel) {
    process_chunk_source(pLevel, TerrainType::Woods);
}

void CrossCraft_WorldGenerator_Generate_Flat(Level* pLevel) {
    process_chunk_source(pLevel, TerrainType::Flat);
}

void CrossCraft_WorldGenerator_Generate_Maze(Level* pLevel) {
    process_chunk_source(pLevel, TerrainType::Maze);
}