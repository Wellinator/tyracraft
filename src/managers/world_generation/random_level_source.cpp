#include "managers/world_generation/random_level_source.hpp"
#include "managers/settings_manager.hpp"
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <utils.hpp>

static constexpr int kOriginalChunkWidth = 4;
static constexpr int kOriginalChunkHeight = 8;
static constexpr float kOriginalNoiseScale = 684.412f;

static int clampInt(const int value, const int minValue, const int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static float clampFloat(const float value, const float minValue,
                        const float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

RandomLevelSource::RandomLevelSource(uint32_t t_seed, TerrainType t_terrain) 
    : seed(t_seed), terrainType(t_terrain) {

    terrainOctaves = 16;
    biomeOctaves = 3;
    terrainNoiseScale = 1.0f;
    heightNoiseOffset = 20.0f;
    heightNoiseDivisor = 40.0f;
    heightScale = 0.6f;
    heightBias = 0.2f;
    waterLevel = 40;
    
    // FastNoiseLite Multi-Layer (Faithful to Engine 0)
    // Optimized for PS2: We use a lower frequency for large features
    const float baseFreq = 0.025f;

    terrainFastNoise1 = new FastNoiseLite(static_cast<int>(seed));
    terrainFastNoise1->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    terrainFastNoise1->SetFractalType(FastNoiseLite::FractalType_FBm);
    terrainFastNoise1->SetFractalOctaves(static_cast<int>(terrainOctaves));
    terrainFastNoise1->SetFractalGain(0.5f);
    terrainFastNoise1->SetFrequency(baseFreq);

    terrainFastNoise2 = new FastNoiseLite(static_cast<int>(seed + 2));
    terrainFastNoise2->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    terrainFastNoise2->SetFractalType(FastNoiseLite::FractalType_FBm);
    terrainFastNoise2->SetFractalOctaves(static_cast<int>(terrainOctaves));
    terrainFastNoise2->SetFractalGain(0.6f);
    terrainFastNoise2->SetFrequency(baseFreq * 1.5f);

    perlinFastNoise1 = new FastNoiseLite(static_cast<int>(seed + 3));
    perlinFastNoise1->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    perlinFastNoise1->SetFractalType(FastNoiseLite::FractalType_FBm);
    perlinFastNoise1->SetFractalOctaves(8);
    perlinFastNoise1->SetFrequency(baseFreq * 0.5f);

    depthFastNoise = new FastNoiseLite(static_cast<int>(seed + 4));
    depthFastNoise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    depthFastNoise->SetFractalType(FastNoiseLite::FractalType_FBm);
    depthFastNoise->SetFractalOctaves(static_cast<int>(terrainOctaves));
    depthFastNoise->SetFractalGain(0.5f);
    depthFastNoise->SetFractalWeightedStrength(-0.5f);
    depthFastNoise->SetFrequency(baseFreq * 0.5f);

    biomeFastNoise = new FastNoiseLite(static_cast<int>(seed + 1));
    biomeFastNoise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    biomeFastNoise->SetFractalType(FastNoiseLite::FractalType_FBm);
    biomeFastNoise->SetFractalOctaves(static_cast<int>(biomeOctaves));
    biomeFastNoise->SetFrequency(0.006f);

    caveFeature = new CaveFeature();
    oakTreeFeature = new TreeFeature(static_cast<uint8_t>(Blocks::OAK_LOG_BLOCK), static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
    birchTreeFeature = new TreeFeature(static_cast<uint8_t>(Blocks::BIRCH_LOG_BLOCK), static_cast<uint8_t>(Blocks::BIRCH_LEAVES_BLOCK));
    
    dirtOreFeature = new OreFeature(static_cast<uint8_t>(Blocks::DIRTY_BLOCK), 32);
    gravelOreFeature = new OreFeature(static_cast<uint8_t>(Blocks::GRAVEL_BLOCK), 32);
    coalOreFeature = new OreFeature(static_cast<uint8_t>(Blocks::COAL_ORE_BLOCK), 16);
    ironOreFeature = new OreFeature(static_cast<uint8_t>(Blocks::IRON_ORE_BLOCK), 8);
    goldOreFeature = new OreFeature(static_cast<uint8_t>(Blocks::GOLD_ORE_BLOCK), 8);
    redstoneOreFeature = new OreFeature(static_cast<uint8_t>(Blocks::REDSTONE_ORE_BLOCK), 7);
    diamondOreFeature = new OreFeature(static_cast<uint8_t>(Blocks::DIAMOND_ORE_BLOCK), 7);
    emeraldOreFeature = new OreFeature(static_cast<uint8_t>(Blocks::EMERALD_ORE_BLOCK), 7);
    
    grassFeature = new PlantFeature(PlantType::Grass, 1);
    flowerFeature = new PlantFeature(PlantType::Flower, 1);
    mushroomFeature = new PlantFeature(PlantType::Mushroom, 1);
    deadBushFeature = new DeadBushFeature();
    cactusFeature = new CactusFeature();
    
    waterLakeFeature = new LakeFeature(static_cast<uint8_t>(Blocks::WATER_BLOCK));
    lavaLakeFeature = new LakeFeature(static_cast<uint8_t>(Blocks::LAVA_BLOCK));

    tallGrassPlantFeature = new TallGrassFeature(static_cast<uint8_t>(Blocks::TALL_GRASS_BLOCK));
    reedsFeature = new ReedsFeature(static_cast<uint8_t>(Blocks::REEDS_BLOCK));
    houseFeature = new HouseFeature();
    canyonFeature = new CanyonFeature();
    sandFeature = new TyraCraft::SandFeature(7, static_cast<uint8_t>(Blocks::SAND_BLOCK));
    gravelFeature = new TyraCraft::SandFeature(6, static_cast<uint8_t>(Blocks::GRAVEL_BLOCK));
}

RandomLevelSource::~RandomLevelSource() {
    delete terrainFastNoise1;
    delete terrainFastNoise2;
    delete perlinFastNoise1;
    delete depthFastNoise;
    delete biomeFastNoise;
    delete caveFeature;
    delete oakTreeFeature;
    delete birchTreeFeature;
    delete dirtOreFeature;
    delete gravelOreFeature;
    delete coalOreFeature;
    delete ironOreFeature;
    delete goldOreFeature;
    delete redstoneOreFeature;
    delete diamondOreFeature;
    delete emeraldOreFeature;
    delete grassFeature;
    delete flowerFeature;
    delete mushroomFeature;
    delete deadBushFeature;
    delete cactusFeature;
    delete tallGrassPlantFeature;
    delete reedsFeature;
    delete houseFeature;
    delete waterLakeFeature;
    delete lavaLakeFeature;
    delete canyonFeature;
    delete sandFeature;
    delete gravelFeature;
}

float RandomLevelSource::sampleTerrainNoise(float x, float z) const {
    if (terrainFastNoise1 != nullptr) {
        return terrainFastNoise1->GetNoise(x, z);
    }
    return 0.0f;
}

float RandomLevelSource::sampleBiomeNoise(float x, float z) const {
    if (biomeFastNoise != nullptr) {
        return biomeFastNoise->GetNoise(x, z);
    }
    return 0.0f;
}

float RandomLevelSource::sampleForestNoise(float x, float z) const {
    return sampleBiomeNoise(x, z);
}

BiomeType RandomLevelSource::resolveBiome(float x, float z) const {
    const float biomeNoise = sampleBiomeNoise(x, z);

    if (biomeNoise < -0.3f) {
        return BiomeType::Desert;
    }
    if (biomeNoise > 0.3f) {
        return BiomeType::Forest;
    }

    return BiomeType::Plains;
}

void RandomLevelSource::getHeights3D(float* buffer, Level* level, int chunkX,
                                     int chunkZ, int xSize, int ySize,
                                     int zSize) {
    const int size3D = xSize * ySize * zSize;
    const int size2D = xSize * zSize;

    for (int i = 0; i < size3D; i++) {
        buffer[i] = 0.0f;
    }

    if (depthFastNoise == nullptr || perlinFastNoise1 == nullptr ||
        terrainFastNoise1 == nullptr || terrainFastNoise2 == nullptr) {
        return;
    }

    float* pnr = new float[size3D];
    float* ar = new float[size3D];
    float* br = new float[size3D];
    float* dr = new float[size2D];

    for (int i = 0; i < size3D; i++) {
        pnr[i] = 0.0f;
        ar[i] = 0.0f;
        br[i] = 0.0f;
    }
    for (int i = 0; i < size2D; i++) {
        dr[i] = 0.0f;
    }

    const int noiseX = chunkX * (CHUNK_SIZE / kOriginalChunkWidth);
    const int noiseZ = chunkZ * (CHUNK_SIZE / kOriginalChunkWidth);

    // FastNoiseLite multi-layer filling
    // We use much larger multipliers to allow the noise to overcome the height gradient ('yOffs')
    // and create mountains.
    for (int xx = 0; xx < xSize; xx++) {
        for (int zz = 0; zz < zSize; zz++) {
            // Coordinates in FastNoiseLite are influenced by frequency set in constructor.
            // We pass noiseX + xx directly which increments by 1 every 4 blocks.
            dr[xx * zSize + zz] = depthFastNoise->GetNoise((float)(noiseX + xx), (float)(noiseZ + zz)) * 16000.0f;
            for (int yy = 0; yy < ySize; yy++) {
                int idx = (xx * zSize + zz) * ySize + yy;
                pnr[idx] = perlinFastNoise1->GetNoise((float)(noiseX + xx), (float)yy, (float)(noiseZ + zz)) * 10.0f;
                ar[idx] = terrainFastNoise1->GetNoise((float)(noiseX + xx), (float)yy, (float)(noiseZ + zz)) * 52428.0f;
                br[idx] = terrainFastNoise2->GetNoise((float)(noiseX + xx), (float)yy, (float)(noiseZ + zz)) * 52428.0f;
            }
        }
    }

    int p = 0;
    int pp = 0;
    const float genDepth = static_cast<float>(level->map.height);

    for (int xx = 0; xx < xSize; xx++) {
        for (int zz = 0; zz < zSize; zz++) {
            float sss = 0.2f;
            float ddd = 0.1f;

            float rdepth = dr[pp] / 8000.0f;
            if (rdepth < 0.0f) {
                rdepth = -rdepth * 0.3f;
            }
            rdepth = rdepth * 3.0f - 2.0f;

            if (rdepth < 0.0f) {
                rdepth /= 2.0f;
                if (rdepth < -1.0f) {
                    rdepth = -1.0f;
                }
                rdepth = rdepth / 1.4f;
                rdepth /= 2.0f;
            } else {
                if (rdepth > 1.0f) {
                    rdepth = 1.0f;
                }
                rdepth /= 8.0f;
            }

            pp++;

            for (int yy = 0; yy < ySize; yy++) {
                float depth = ddd + rdepth * 0.2f;
                depth = depth * static_cast<float>(ySize) / 16.0f;

                const float yCenter = (static_cast<float>(ySize) / 2.0f) +
                                      (depth * 6.0f);
                float yOffs = (static_cast<float>(yy) - yCenter) * 2.4f *
                              128.0f / genDepth / sss;
                if (yOffs < 0.0f) {
                    yOffs *= 4.0f;
                }

                const float bb = ar[p] / 512.0f;
                const float cc = br[p] / 512.0f;
                const float v = (pnr[p] / 10.0f + 1.0f) / 2.0f;

                float val = 0.0f;
                if (v < 0.0f) {
                    val = bb;
                } else if (v > 1.0f) {
                    val = cc;
                } else {
                    val = bb + (cc - bb) * v;
                }

                val -= yOffs;

                if (yy > ySize - 4) {
                    const float slide =
                        static_cast<float>(yy - (ySize - 4)) / 3.0f;
                    val = val * (1.0f - slide) + (-10.0f * slide);
                }

                buffer[p] = val;
                p++;
            }
        }
    }

    delete[] pnr;
    delete[] ar;
    delete[] br;
    delete[] dr;
}

float RandomLevelSource::computeEdgeFalloff(Level* level, int blockX,
                                            int blockZ) const {
    const int worldSizeX = static_cast<int>(level->map.width);
    const int worldSizeZ = static_cast<int>(level->map.length);
    const int falloffStart = 16;
    const float falloffMax = 128.0f;

    const int distLeft = blockX;
    const int distRight = (worldSizeX - 1) - blockX;
    const int distTop = blockZ;
    const int distBottom = (worldSizeZ - 1) - blockZ;

    int emin = distLeft;
    if (distRight < emin) emin = distRight;
    if (distTop < emin) emin = distTop;
    if (distBottom < emin) emin = distBottom;

    if (emin < 0) {
        emin = 0;
    }

    if (emin < falloffStart) {
        const int falloff = falloffStart - emin;
        return (static_cast<float>(falloff) / static_cast<float>(falloffStart)) *
               falloffMax;
    }

    return 0.0f;
}

void RandomLevelSource::generateChunk(Level* level, int chunkX, int chunkZ) {
    if (terrainType == TerrainType::Original || terrainType == TerrainType::Island) {
        applyHeights(level, chunkX, chunkZ);
        applySurfaces(level, chunkX, chunkZ);
        fillOceans(level, chunkX, chunkZ);
    } else if (terrainType == TerrainType::Floating) {
        // Simplified floating implementation could be added here
        // Originally FastNoiseLite logic applied just shifted
        TYRA_WARN("RandomLevelSource Floating generation not yet fully ported!");
    } else if (terrainType == TerrainType::Maze) {
        // Handled by Maze generator, but kept here for fallback
    }
}

void RandomLevelSource::applyHeights(Level* level, int chunkX, int chunkZ) {
    const int xChunks = CHUNK_SIZE / kOriginalChunkWidth;
    const int yChunks = level->map.height / kOriginalChunkHeight;
    const int xSize = xChunks + 1;
    const int ySize = yChunks + 1;
    const int zSize = xChunks + 1;

    const int densityCount = xSize * ySize * zSize;
    float* density = new float[densityCount];
    getHeights3D(density, level, chunkX, chunkZ, xSize, ySize, zSize);

    const int startX = chunkX * CHUNK_SIZE;
    const int startZ = chunkZ * CHUNK_SIZE;
    const int effectiveWaterLevel =
        clampInt(static_cast<int>(waterLevel), 1, level->map.height - 1);

    for (int xc = 0; xc < xChunks; xc++) {
        for (int zc = 0; zc < xChunks; zc++) {
            for (int yc = 0; yc < yChunks; yc++) {
                const float yStep = 1.0f / static_cast<float>(kOriginalChunkHeight);

                float s0 = density[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s1 = density[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 0)];
                float s2 = density[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s3 = density[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 0)];

                float s0a =
                    (density[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 1)] - s0) * yStep;
                float s1a =
                    (density[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 1)] - s1) * yStep;
                float s2a =
                    (density[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 1)] - s2) * yStep;
                float s3a =
                    (density[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 1)] - s3) * yStep;

                for (int y = 0; y < kOriginalChunkHeight; y++) {
                    const float xStep = 1.0f / static_cast<float>(kOriginalChunkWidth);

                    float _s0 = s0;
                    float _s1 = s1;
                    float _s0a = (s2 - s0) * xStep;
                    float _s1a = (s3 - s1) * xStep;

                    for (int x = 0; x < kOriginalChunkWidth; x++) {
                        const float zStep =
                            1.0f / static_cast<float>(kOriginalChunkWidth);

                        float val = _s0;
                        const float vala = (_s1 - _s0) * zStep;

                        for (int z = 0; z < kOriginalChunkWidth; z++) {
                            const int worldX =
                                startX + x + (xc * kOriginalChunkWidth);
                            const int worldY =
                                (yc * kOriginalChunkHeight) + y;
                            const int worldZ =
                                startZ + z + (zc * kOriginalChunkWidth);

                            if (level->BoundCheckMap(worldX, worldY, worldZ)) {
                                const float comp =
                                    computeEdgeFalloff(level, worldX, worldZ);

                                if (val > comp) {
                                    level->SetBlockInMap(worldX, worldY, worldZ,
                                                            static_cast<uint8_t>(
                                                                Blocks::STONE_BLOCK));
                                } else if (worldY < effectiveWaterLevel) {
                                    level->SetBlockInMap(worldX, worldY, worldZ,
                                                            static_cast<uint8_t>(
                                                                Blocks::WATER_BLOCK));
                                    level->SetLiquidDataToMap(
                                        worldX, worldY, worldZ,
                                        static_cast<uint8_t>(
                                            LiquidLevel::Percent100));
                                } else {
                                    level->SetBlockInMap(worldX, worldY, worldZ,
                                                            static_cast<uint8_t>(
                                                                Blocks::AIR_BLOCK));
                                }
                            }

                            val += vala;
                        }

                        _s0 += _s0a;
                        _s1 += _s1a;
                    }

                    s0 += s0a;
                    s1 += s1a;
                    s2 += s2a;
                    s3 += s3a;
                }
            }
        }
    }

    delete[] density;
}

void RandomLevelSource::applyStrata(Level* level, int chunkX, int chunkZ) {
    int startX = chunkX * CHUNK_SIZE;
    int startZ = chunkZ * CHUNK_SIZE;

    for (int x = startX; x < startX + CHUNK_SIZE; x++) {
        for (int z = startZ; z < startZ + CHUNK_SIZE; z++) {
            if (!level->BoundCheckMap(x, 0, z)) continue;

            if (level->GetBlockFromMap(x, 0, z) == static_cast<uint8_t>(Blocks::STONE_BLOCK)) {
                level->SetBlockInMap(x, 0, z, static_cast<uint8_t>(Blocks::BEDROCK_BLOCK));
            }
        }
    }
}

void RandomLevelSource::applySurfaces(Level* level, int chunkX, int chunkZ) {
    const int startX = chunkX * CHUNK_SIZE;
    const int startZ = chunkZ * CHUNK_SIZE;
    const int effectiveWaterLevel =
        clampInt(static_cast<int>(waterLevel), 1, level->map.height - 1);

    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            const int worldX = startX + lx;
            const int worldZ = startZ + lz;

            int runDepth = static_cast<int>(
                perlinFastNoise1->GetNoise((float)worldX, (float)worldZ) * 3.0f + 3.0f +
                (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) *
                    0.25f);

            int run = -1;
            const BiomeType biome =
                resolveBiome(static_cast<float>(worldX), static_cast<float>(worldZ));
            const BiomeConfig& biomeConfig = getBiomeConfig(biome);

            uint8_t topMaterial = biomeConfig.topBlock;
            uint8_t fillMaterial = biomeConfig.fillBlock;
            uint8_t underFillMaterial = biomeConfig.underFillBlock;

            for (int y = level->map.height - 1; y >= 0; y--) {
                if (y <= 1 + (rand() % 2)) {
                    level->SetBlockInMap(
                        worldX, y, worldZ,
                        static_cast<uint8_t>(Blocks::BEDROCK_BLOCK));
                    continue;
                }

                const uint8_t old = level->GetBlockFromMap(worldX, y, worldZ);
                if (old == static_cast<uint8_t>(Blocks::AIR_BLOCK) ||
                    old == static_cast<uint8_t>(Blocks::WATER_BLOCK)) {
                    run = -1;
                    continue;
                }

                if (old != static_cast<uint8_t>(Blocks::STONE_BLOCK)) {
                    continue;
                }

                if (run == -1) {
                    if (runDepth <= 0) {
                        topMaterial = static_cast<uint8_t>(Blocks::AIR_BLOCK);
                        fillMaterial =
                            static_cast<uint8_t>(Blocks::STONE_BLOCK);
                        underFillMaterial =
                            static_cast<uint8_t>(Blocks::STONE_BLOCK);
                    } else if (y >= effectiveWaterLevel - 4 &&
                                y <= effectiveWaterLevel + 1) {
                        topMaterial = biomeConfig.topBlock;
                        fillMaterial = biomeConfig.fillBlock;
                        underFillMaterial = biomeConfig.underFillBlock;
                    }

                    if (y < effectiveWaterLevel &&
                        topMaterial ==
                            static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
                        topMaterial =
                            static_cast<uint8_t>(Blocks::WATER_BLOCK);
                    }

                    run = runDepth;
                    if (y >= effectiveWaterLevel - 1) {
                        level->SetBlockInMap(worldX, y, worldZ, topMaterial);
                        if (topMaterial ==
                            static_cast<uint8_t>(Blocks::WATER_BLOCK)) {
                            level->SetLiquidDataToMap(
                                worldX, y, worldZ,
                                static_cast<uint8_t>(
                                    LiquidLevel::Percent100));
                        }
                    } else {
                        level->SetBlockInMap(worldX, y, worldZ,
                                                fillMaterial);
                    }
                } else if (run > 0) {
                    run--;
                    level->SetBlockInMap(worldX, y, worldZ, fillMaterial);

                    if (run == 0 &&
                        fillMaterial ==
                            static_cast<uint8_t>(Blocks::SAND_BLOCK)) {
                        run = rand() % 4;
                        fillMaterial = underFillMaterial;
                    }
                }
            }
        }
    }
}

void RandomLevelSource::fillOceans(Level* level, int chunkX, int chunkZ) {
    int startX = chunkX * CHUNK_SIZE;
    int startZ = chunkZ * CHUNK_SIZE;
    const int effectiveWaterLevel = clampInt(static_cast<int>(waterLevel), 1, level->map.height - 1);

    for (int x = startX; x < startX + CHUNK_SIZE; x++) {
        for (int z = startZ; z < startZ + CHUNK_SIZE; z++) {
            if (!level->BoundCheckMap(x, 0, z)) continue;

            for (int y = 0; y <= effectiveWaterLevel; y++) {
                if (level->GetBlockFromMap(x, y, z) == static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
                    level->SetBlockInMap(x, y, z, static_cast<uint8_t>(Blocks::WATER_BLOCK));
                    level->SetLiquidDataToMap(x, y, z, static_cast<uint8_t>(LiquidLevel::Percent100));
                }
            }
            
            // Generate ice
            float temperature = sampleBiomeNoise((float)x, (float)z);
            if (level->GetBlockFromMap(x, effectiveWaterLevel, z) == static_cast<uint8_t>(Blocks::WATER_BLOCK) && temperature > 0.4f) {
                // Ignore ice block for TyraCraft
            }
        }
    }
}

void RandomLevelSource::carve(Level* level, int chunkX, int chunkZ) {
    if (terrainType == TerrainType::Flat) return;

    int startX = chunkX * CHUNK_SIZE;
    int startZ = chunkZ * CHUNK_SIZE;

    // Set seed for deterministic chunk post-processing
    srand(seed + chunkX * 341873128712ull + chunkZ * 132897987541ull);

    // 1. Caves
    if (terrainType != TerrainType::Woods) {
        int caveAttempts = 0;

        if ((rand() % 100) < 65) {
            caveAttempts = 1 + (rand() % 2);
        }
        if ((rand() % 100) < 25) {
            caveAttempts += 1;
        }

        for (int i = 0; i < caveAttempts; i++) {
            const int cx = startX + (rand() % CHUNK_SIZE);
            const int cz = startZ + (rand() % CHUNK_SIZE);

            int topY = level->map.height - 1;
            while (topY > 0 &&
                   level->GetBlockFromMap(cx, topY, cz) ==
                       static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
                topY--;
            }

            if (topY < 12) {
                continue;
            }

            const int minY = 6;
            const int maxY = topY - 3;
            if (maxY <= minY) {
                continue;
            }

            const int yRange = (maxY - minY) + 1;
            const int ry0 = rand() % yRange;
            const int ry1 = rand() % yRange;
            const int cy = minY + ((ry0 + ry1) / 2);

            caveFeature->place(level, cx, cy, cz);
        }

        // 1.1 Ravines (CanyonFeature)
        if (rand() % 50 == 0) {
            const int rx = startX + (rand() % CHUNK_SIZE);
            const int rz = startZ + (rand() % CHUNK_SIZE);

            int topY = level->map.height - 1;
            while (topY > 0 && level->GetBlockFromMap(rx, topY, rz) ==
                                   static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
                topY--;
            }

            if (topY > 20) {
                const int ry = 10 + (rand() % (topY - 10));
                canyonFeature->place(level, rx, ry, rz);
            }
        }
    }

    // 2. Houses (Rare) - Part of carving/structure phase
    if (rand() % 50 == 0) {
        int hx = startX + (rand() % CHUNK_SIZE);
        int hz = startZ + (rand() % CHUNK_SIZE);
        int hy = level->map.height - 1;
        houseFeature->place(level, hx, hy, hz);
    }

    // 3. Lakes
    if (rand() % 8 == 0) {
        int lx = startX + (rand() % CHUNK_SIZE);
        int lz = startZ + (rand() % CHUNK_SIZE);
        int ly = rand() % level->map.height;
        waterLakeFeature->place(level, lx, ly, lz);
    }

    if (rand() % 16 == 0) {
        int lx = startX + (rand() % CHUNK_SIZE);
        int lz = startZ + (rand() % CHUNK_SIZE);
        int ly = rand() % (level->map.height / 2);
        lavaLakeFeature->place(level, lx, ly, lz);
    }

    // 3.1 Sand and Gravel (SandFeature)
    for (int i = 0; i < 3; i++) {
        int sx = startX + (rand() % CHUNK_SIZE);
        int sz = startZ + (rand() % CHUNK_SIZE);
        int sy = level->map.height - 1;
        while (sy > 0 && level->GetBlockFromMap(sx, sy, sz) == static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
            sy--;
        }
        sandFeature->place(level, sx, sy, sz);
    }
    if (rand() % 2 == 0) {
        int gx = startX + (rand() % CHUNK_SIZE);
        int gz = startZ + (rand() % CHUNK_SIZE);
        int gy = level->map.height - 1;
        while (gy > 0 && level->GetBlockFromMap(gx, gy, gz) == static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
            gy--;
        }
        gravelFeature->place(level, gx, gy, gz);
    }

    // 4. Ores
    auto placeOre = [&](OreFeature* feature, int count, int minY, int maxY) {
        for (int i = 0; i < count; i++) {
            int ox = startX + (rand() % CHUNK_SIZE);
            int oy = minY + (rand() % (maxY - minY + 1));
            int oz = startZ + (rand() % CHUNK_SIZE);
            feature->place(level, ox, oy, oz);
        }
    };

    placeOre(dirtOreFeature, 5, 0, level->map.height - 1);
    placeOre(gravelOreFeature, 3, 0, level->map.height - 1);
    placeOre(coalOreFeature, 5, 0, level->map.height - 1);
    placeOre(ironOreFeature, 5, 0, level->map.height / 2);

    if (rand() % 2 == 0) placeOre(goldOreFeature, 1, 0, level->map.height / 4);
    if (rand() % 2 == 0) placeOre(redstoneOreFeature, 1, 0, level->map.height / 8);
    if (rand() % 4 == 0) placeOre(diamondOreFeature, 1, 0, level->map.height / 8);
    if (rand() % 4 == 0) placeOre(emeraldOreFeature, 1, 0, level->map.height / 8);
}

void RandomLevelSource::postProcess(Level* level, int chunkX, int chunkZ) {
    if (terrainType == TerrainType::Flat) return;

    int startX = chunkX * CHUNK_SIZE;
    int startZ = chunkZ * CHUNK_SIZE;

    // Set seed for deterministic chunk post-processing
    srand(seed + chunkX * 341873128712ull + chunkZ * 132897987541ull);

    // 5. Surface Decoration (Trees, Plants)
    const float biomeSampleX = static_cast<float>(startX + (CHUNK_SIZE / 2));
    const float biomeSampleZ = static_cast<float>(startZ + (CHUNK_SIZE / 2));
    const BiomeType biome = resolveBiome(biomeSampleX, biomeSampleZ);
    const BiomeConfig& biomeConfig = getBiomeConfig(biome);
    float temperature = sampleForestNoise((float)startX, (float)startZ);

    int treesToGen = 0;
    if (terrainType == TerrainType::Woods) {
        treesToGen = 5;
    } else if (biomeConfig.treeCount < 0) {
        treesToGen = 0;
    } else if (biomeConfig.treeCount == 0) {
        treesToGen = (rand() % 4 == 0) ? 1 : 0;
    } else {
        treesToGen = biomeConfig.treeCount;
    }

    for (int i = 0; i < treesToGen; i++) {
        int tx = startX + (rand() % CHUNK_SIZE);
        int tz = startZ + (rand() % CHUNK_SIZE);

        int ty = level->map.height - 1;
        while (ty > 0 && level->GetBlockFromMap(tx, ty, tz) == static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
            ty--;
        }
        ty++;

        if (ty < 20) continue;

        if (rand() % 100 < biomeConfig.birchChancePercent) {
            birchTreeFeature->place(level, tx, ty, tz);
        } else {
            oakTreeFeature->place(level, tx, ty, tz);
        }
    }

    for (u8 i = 0; i < biomeConfig.grassCount; i++) {
        if (rand() % 4 == 0) {
            const int px = startX + (rand() % CHUNK_SIZE);
            const int pz = startZ + (rand() % CHUNK_SIZE);

            if (rand() % 4 == 0) {
                tallGrassPlantFeature->place(level, px, level->map.height - 1, pz);
            } else {
                grassFeature->place(level, px, 0, pz);
            }
        }
    }

    // 6. Reeds (Sugar Cane)
    if (rand() % 5 == 0) {
        int rx = startX + (rand() % CHUNK_SIZE);
        int rz = startZ + (rand() % CHUNK_SIZE);
        reedsFeature->place(level, rx, level->map.height - 1, rz);
    }

    for (u8 i = 0; i < biomeConfig.flowerCount; i++) {
        if (rand() % 5 == 0) {
            const int px = startX + (rand() % CHUNK_SIZE);
            const int pz = startZ + (rand() % CHUNK_SIZE);
            flowerFeature->place(level, px, 0, pz);
        }
    }

    for (u8 i = 0; i < biomeConfig.deadBushCount; i++) {
        const int px = startX + (rand() % CHUNK_SIZE);
        const int pz = startZ + (rand() % CHUNK_SIZE);
        deadBushFeature->place(level, px, 0, pz);
    }

    for (u8 i = 0; i < biomeConfig.cactusCount; i++) {
        const int px = startX + (rand() % CHUNK_SIZE);
        const int pz = startZ + (rand() % CHUNK_SIZE);
        cactusFeature->place(level, px, 0, pz);
    }

    if (temperature > 0.4f && rand() % 4 == 0) {
        int px = startX + (rand() % CHUNK_SIZE);
        int pz = startZ + (rand() % CHUNK_SIZE);
        int py = rand() % level->map.height;
        mushroomFeature->place(level, px, py, pz);
    }
}
