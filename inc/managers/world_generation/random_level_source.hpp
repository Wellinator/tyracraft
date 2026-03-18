#pragma once

#include "managers/world_generation/chunk_source.hpp"
#include "managers/world_generation/features/cave_feature.hpp"
#include "managers/world_generation/features/tree_feature.hpp"
#include "managers/world_generation/features/ore_feature.hpp"
#include "managers/world_generation/features/plant_feature.hpp"
#include "managers/world_generation/features/lake_feature.hpp"
#include "managers/world_generation/features/dead_bush_feature.hpp"
#include "managers/world_generation/features/cactus_feature.hpp"
#include "managers/world_generation/features/tall_grass_feature.hpp"
#include "managers/world_generation/features/reeds_feature.hpp"
#include "managers/world_generation/features/house_feature.hpp"
#include "managers/world_generation/features/canyon_feature.hpp"
#include "managers/world_generation/features/sand_feature.hpp"
#include "managers/world_generation/biome.hpp"
#include "3libs/FastNoiseLite/FastNoiseLite.h"
#include <constants.hpp>

class RandomLevelSource : public ChunkSource {
private:
    FastNoiseLite* terrainFastNoise1;
    FastNoiseLite* terrainFastNoise2;
    FastNoiseLite* perlinFastNoise1;
    FastNoiseLite* depthFastNoise;
    FastNoiseLite* biomeFastNoise;
    uint32_t seed;

    u8 terrainOctaves;
    u8 biomeOctaves;
    float terrainNoiseScale;
    float heightNoiseOffset;
    float heightNoiseDivisor;
    float heightScale;
    float heightBias;
    u16 waterLevel;

    CaveFeature* caveFeature;
    TreeFeature* oakTreeFeature;
    TreeFeature* birchTreeFeature;
    OreFeature* dirtOreFeature;
    OreFeature* gravelOreFeature;
    OreFeature* coalOreFeature;
    OreFeature* ironOreFeature;
    OreFeature* goldOreFeature;
    OreFeature* redstoneOreFeature;
    OreFeature* diamondOreFeature;
    OreFeature* emeraldOreFeature;
    PlantFeature* grassFeature;
    PlantFeature* flowerFeature;
    PlantFeature* mushroomFeature;
    DeadBushFeature* deadBushFeature;
    CactusFeature* cactusFeature;
    TallGrassFeature* tallGrassPlantFeature;
    ReedsFeature* reedsFeature;
    HouseFeature* houseFeature;
    LakeFeature* waterLakeFeature;
    LakeFeature* lavaLakeFeature;
    CanyonFeature* canyonFeature;
    TyraCraft::SandFeature* sandFeature;
    TyraCraft::SandFeature* gravelFeature;

    TerrainType terrainType;

public:
    RandomLevelSource(uint32_t seed, TerrainType terrain);
    ~RandomLevelSource() override;

    void generateChunk(Level* level, int chunkX, int chunkZ) override;
    void postProcess(Level* level, int chunkX, int chunkZ) override;

private:
    float sampleTerrainNoise(float x, float z) const;
    float sampleBiomeNoise(float x, float z) const;
    float sampleForestNoise(float x, float z) const;
    BiomeType resolveBiome(float x, float z) const;
    void getHeights3D(float* buffer, Level* level, int chunkX, int chunkZ,
                      int xSize, int ySize, int zSize);
    float computeEdgeFalloff(Level* level, int blockX, int blockZ) const;
    void applyHeights(Level* level, int chunkX, int chunkZ);
    void applySurfaces(Level* level, int chunkX, int chunkZ);
    void applyStrata(Level* level, int chunkX, int chunkZ);
    void fillOceans(Level* level, int chunkX, int chunkZ);
};
