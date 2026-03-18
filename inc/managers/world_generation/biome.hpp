#pragma once

#include <constants.hpp>

enum class BiomeType : u8 {
    Plains = 0,
    Forest = 1,
    Desert = 2
};

struct BiomeConfig {
    u8 topBlock;
    u8 fillBlock;
    u8 underFillBlock;
    s8 treeCount;
    u8 grassCount;
    u8 flowerCount;
    u8 deadBushCount;
    u8 cactusCount;
    u8 birchChancePercent;
};

static constexpr BiomeConfig kBiomePlainsConfig = {
    static_cast<u8>(Blocks::GRASS_BLOCK),
    static_cast<u8>(Blocks::DIRTY_BLOCK),
    static_cast<u8>(Blocks::STONE_BLOCK),
    0,
    10,
    4,
    0,
    0,
    0,
};

static constexpr BiomeConfig kBiomeForestConfig = {
    static_cast<u8>(Blocks::GRASS_BLOCK),
    static_cast<u8>(Blocks::DIRTY_BLOCK),
    static_cast<u8>(Blocks::STONE_BLOCK),
    5,
    2,
    1,
    0,
    0,
    20,
};

static constexpr BiomeConfig kBiomeDesertConfig = {
    static_cast<u8>(Blocks::SAND_BLOCK),
    static_cast<u8>(Blocks::SAND_BLOCK),
    static_cast<u8>(Blocks::SANDSTONE_BLOCK),
    -1,
    0,
    0,
    2,
    1,
    0,
};

inline const BiomeConfig& getBiomeConfig(const BiomeType biome) {
    switch (biome) {
        case BiomeType::Forest:
            return kBiomeForestConfig;
        case BiomeType::Desert:
            return kBiomeDesertConfig;
        case BiomeType::Plains:
        default:
            return kBiomePlainsConfig;
    }
}
