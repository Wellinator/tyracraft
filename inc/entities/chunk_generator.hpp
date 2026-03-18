#pragma once

#include "entities/chunk_provider.hpp"
#include "entities/level.hpp"
#include "constants.hpp"
#include "managers/world_generation/chunk_source.hpp"

class ChunkGenerator : public ChunkDataSource {
 public:
  ChunkGenerator(Level* level, uint32_t seed, WorldType worldType);
  ~ChunkGenerator() override;

  LevelChunk* getChunk(int x, int z) override;
  void saveChunk(LevelChunk* chunk) override;

  void generateTerrain(int x, int z) override;
  void carve(int x, int z) override;
  void decorate(int x, int z) override;

 private:
  Level* level;
  ChunkSource* terrainSource;

  TerrainType resolveTerrainType(WorldType worldType) const;
  uint32_t toChunkIndex(int x, int z) const;
};
