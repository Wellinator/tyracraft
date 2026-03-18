#include "entities/chunk_generator.hpp"

#include "managers/world_generation/random_level_source.hpp"
#include "managers/world_generation/flat_level_source.hpp"
#include "managers/world_generation/maze_level_source.hpp"

ChunkGenerator::ChunkGenerator(Level* level, uint32_t seed,
                               WorldType worldType)
    : level(level), terrainSource(nullptr) {
  const TerrainType terrain = resolveTerrainType(worldType);

  if (terrain == TerrainType::Flat) {
    terrainSource = new TyraCraft::FlatLevelSource();
  } else if (terrain == TerrainType::Maze) {
    terrainSource = new TyraCraft::MazeLevelSource(seed);
  } else {
    // ORIGINAL/WOODS/ISLAND/FLOATING should all remain procedural.
    terrainSource = new RandomLevelSource(seed, terrain);
  }
}

ChunkGenerator::~ChunkGenerator() {
  if (terrainSource) {
    delete terrainSource;
    terrainSource = nullptr;
  }
}

LevelChunk* ChunkGenerator::getChunk(int x, int z) {
  if (!level || !terrainSource) return nullptr;

  const uint32_t index = toChunkIndex(x, z);
  if (index >= OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD) return nullptr;

  if (level->map.chunks[index] == nullptr) {
    // Register chunk first so generation that touches this column can recurse
    // safely via Level::getChunk without re-entering provider generation.
    level->map.chunks[index] = new LevelChunk(x * CHUNK_SIZE, z * CHUNK_SIZE);

    terrainSource->generateChunk(level, x, z);
    terrainSource->carve(level, x, z);
    terrainSource->postProcess(level, x, z);

    level->map.chunks[index]->isDirty = false;
  }

  return level->map.chunks[index];
}

void ChunkGenerator::generateTerrain(int x, int z) {
  if (!level || !terrainSource) return;
  const uint32_t index = toChunkIndex(x, z);
  if (index >= OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD) return;

  if (level->map.chunks[index] == nullptr) {
    level->map.chunks[index] = new LevelChunk(x * CHUNK_SIZE, z * CHUNK_SIZE);
  }

  terrainSource->generateChunk(level, x, z);
  level->map.chunks[index]->isDirty = true;
}

void ChunkGenerator::carve(int x, int z) {
  if (!level || !terrainSource) return;
  const uint32_t index = toChunkIndex(x, z);
  if (index >= OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD) return;

  if (level->map.chunks[index] != nullptr) {
    terrainSource->carve(level, x, z);
    level->map.chunks[index]->isDirty = true;
  }
}

void ChunkGenerator::decorate(int x, int z) {
  if (!level || !terrainSource) return;
  const uint32_t index = toChunkIndex(x, z);
  if (index >= OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD) return;

  // Decorate assumes chunk is already loaded/generated.
  if (level->map.chunks[index] != nullptr) {
    terrainSource->postProcess(level, x, z);
    level->map.chunks[index]->isDirty = true;
  }
}

void ChunkGenerator::saveChunk(LevelChunk* chunk) {
  (void)chunk;
}

TerrainType ChunkGenerator::resolveTerrainType(WorldType worldType) const {
  switch (worldType) {
    case WorldType::WORLD_TYPE_FLAT:
      return TerrainType::Flat;
    case WorldType::WORLD_TYPE_ISLAND:
      return TerrainType::Island;
    case WorldType::WORLD_TYPE_WOODS:
      return TerrainType::Woods;
    case WorldType::WORLD_TYPE_FLOATING:
      return TerrainType::Floating;
    case WorldType::WORLD_MINI_GAME_MAZECRAFT:
      return TerrainType::Maze;
    case WorldType::WORLD_TYPE_ORIGINAL:
    default:
      return TerrainType::Original;
  }
}

uint32_t ChunkGenerator::toChunkIndex(int x, int z) const {
  const uint32_t chunkWidth = level->map.width / CHUNK_SIZE;
  return static_cast<uint32_t>(z) * chunkWidth + static_cast<uint32_t>(x);
}
