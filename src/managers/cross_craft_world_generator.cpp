#define FNL_IMPL
#include <managers/cross_craft_world_generator.hpp>
#define _USE_MATH_DEFINES

static fnl_state state;
static int32_t worldgen_seed;

void CrossCraft_WorldGenerator_Init(int32_t seed) {
  state = fnlCreateState();
  state.seed = seed;
  state.noise_type = FNL_NOISE_PERLIN;
  state.frequency = 0.8f;
  worldgen_seed = seed;
}

float noise3d(float x, float y, float z) {
  float freq = 0.05f;
  return fnlGetNoise3D(&state, x * freq, y * freq, z * freq);
}

// AMPLIFIED: amp = 2
float octave_noise(uint8_t octaves, float x, float y, uint16_t seed_modifier) {
  float sum = 0;
  float amp = 1.5f;
  float freq = 1;

  for (uint8_t i = 0; i < octaves; i++) {
    state.seed += seed_modifier;
    sum += fnlGetNoise2D(&state, x * freq, y * freq) * amp;

    amp *= 2;
    freq /= 2;
  }

  state.seed = worldgen_seed;

  return sum;
}

float combined_noise(uint8_t octaves, float x, float y,
                     uint16_t seed_modifier) {
  float n2 = octave_noise(octaves, x, y, seed_modifier);
  return octave_noise(octaves, x + n2, y, seed_modifier);
}

float noise1(float x, float y) { return combined_noise(8, x, y, 1); }

float noise2(float x, float y) { return combined_noise(8, x, y, 2); }

float noise3(float x, float y) { return octave_noise(6, x, y, 0); }

// Height of water
const int waterLevel = 32;

/**
 * Generates a heightmap
 * @param heightmap Map to generate
 * @param length Length of map (X)
 * @param width Width of map (Z)
 */
void create_heightmap(int16_t* heightmap, uint16_t length, uint16_t width) {
  for (int x = 0; x < length; x++) {
    for (int z = 0; z < width; z++) {
      float xf = (float)x;
      float zf = (float)z;

      float heightLow = noise1(xf * 1.3f, zf * 1.3f) / 6 - 4;
      float heightHigh = noise2(xf * 1.3f, zf * 1.3f) / 5 + 6;

      float hResult = 0.0f;

      if (noise3(xf, zf) / 8 > 0) {
        hResult = heightLow;
      } else {
        if (heightHigh > heightLow)
          hResult = heightHigh;
        else
          hResult = heightLow;
      }

      hResult /= 2.0f;

      if (hResult < 0) hResult *= 0.8f;

      heightmap[x + z * length] = (int16_t)(hResult + (float)waterLevel);
    }
  }
}

/**
 * Smooths out the heightmap
 * @param heightmap Map to smooth
 * @param length Length of map (X)
 * @param width Length of map (Z)
 */
void smooth_heightmap(int16_t* heightmap, uint16_t length, uint16_t width) {
  for (int x = 0; x < length; x++) {
    for (int z = 0; z < width; z++) {
      float a = noise1((float)x * 2, (float)z * 2) / 8.0f;
      float b = (noise2((float)x * 2, (float)z * 2) > 0) ? 1 : 0;

      if (a > 2) {
        float c = ((float)heightmap[x + z * length] - b) / 2;
        float d = (float)((int)c * 2) + b;

        heightmap[x + z * length] = (int16_t)d;
      }
    }
  }
}

void smooth_distance(int16_t* heightmap, uint16_t length, uint16_t width) {
  int midl = length / 2;
  int midw = width / 2;

  float maxDist = sqrtf(length * length + width * width) / 2;

  for (int x = 0; x < length; x++) {
    for (int z = 0; z < width; z++) {
      int diffX = midl - x;
      int diffZ = midw - z;

      float dist = sqrtf(diffX * diffX + diffZ * diffZ) / maxDist;

      heightmap[x + z * length] =
          ((1 - dist) + 0.2f) * (float)heightmap[x + z * length];
    }
  }
}

void create_strata(LevelMap* map, const int16_t* heightmap) {
  for (uint16_t x = 0; x < map->length; x++) {
    for (uint16_t z = 0; z < map->width; z++) {
      float dirt_thickness = octave_noise(8, x, z, 0) / 24.0f - 4.0f;
      int dirt_transition = heightmap[x + z * map->length];
      int stone_transition = dirt_transition + dirt_thickness;

      for (int y = 0; y < map->height; y++) {
        u8 block_type = static_cast<uint8_t>(Blocks::AIR_BLOCK);

        if (y == 0) {
          block_type = static_cast<uint8_t>(Blocks::BEDROCK_BLOCK);
        } else if (y == 1) {
          block_type = rand() % 2 == 0
                           ? static_cast<uint8_t>(Blocks::BEDROCK_BLOCK)
                           : static_cast<uint8_t>(Blocks::STONE_BLOCK);
        } else if (y <= stone_transition) {
          block_type = static_cast<uint8_t>(Blocks::STONE_BLOCK);
        } else if (y <= dirt_transition) {
          block_type = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
        }

        SetBlockInMap(map, x, y, z, block_type);
      }
    }
  }
}

void create_strata2(LevelMap* map, const int16_t* heightmap,
                    const int16_t* heightmap2) {
  for (uint16_t x = 0; x < map->length; x++) {
    for (uint16_t z = 0; z < map->width; z++) {
      float dirt_thickness = octave_noise(8, x, z, 0) / 24.0f - 4.0f;
      int dirt_transition = heightmap[x + z * map->length];
      if (dirt_transition >= 63 || dirt_transition <= 0) continue;

      int stone_transition = dirt_transition + dirt_thickness;

      int start = heightmap2[x + z * map->length];

      for (int y = dirt_transition; y >= start; y--) {
        int block_type = static_cast<uint8_t>(Blocks::AIR_BLOCK);

        if (y <= stone_transition) {
          block_type = static_cast<uint8_t>(Blocks::STONE_BLOCK);
        } else if (y <= dirt_transition) {
          block_type = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
        }

        if (GetBlockFromMap(map, x, y, z) ==
            static_cast<uint8_t>(Blocks::AIR_BLOCK))
          break;

        SetBlockInMap(map, x, y, z, block_type);
      }
    }
  }
}

// The fillOblateSpheroid function takes in a pointer to a LevelMap structure,
// the center coordinates (center_x, center_y, center_z) of an oblate spheroid,
// the radius of the spheroid, and a block type represented as a uint8_t value.
// It then iterates over all points within the given radius of the center and
// checks if the point is within the spheroid and has a block type of 1.
// If so, it sets the block at that point to the given block type.
// This function is useful for filling an oblate spheroid with a specific block
// type in a level map.
void fillOblateSpheroid(LevelMap* map, int center_x, int center_y, int center_z,
                        int radius, uint8_t blk) {
  for (int x = center_x - radius; x <= center_x + radius; x++) {
    for (int y = center_y - radius; y <= center_y + radius; y++) {
      for (int z = center_z - radius; z <= center_z + radius; z++) {
        // Check if point is within bounds of map and has block type of 1
        if (BoundCheckMap(map, x, y, z) &&
            GetBlockFromMap(map, x, y, z) ==
                static_cast<uint8_t>(Blocks::STONE_BLOCK)) {
          // Set block at point to given block type
          SetBlockInMap(map, x, y, z, blk);
        }
      }
    }
  }
}

void create_caves(LevelMap* map) {
  int num_caves = (map->length * map->height * map->width) / 8192;
  for (int i = 0; i < num_caves; i++) {
    int cave_x = rand() % map->length;
    int cave_y = rand() % map->height;
    int cave_z = rand() % map->width;

    // Generate a random cave length
    int cave_length =
        (int)((rand() / (float)RAND_MAX + rand() / (float)RAND_MAX) * 200.0f);

    // Generate random initial angles and rate of change
    float theta = rand() / (float)RAND_MAX * M_PI * 2.0f;
    float delta_theta = 0.0f;
    float phi = rand() / (float)RAND_MAX * M_PI * 2.0f;
    float delta_phi = 0.0f;

    // Generate a random cave radius
    float cave_radius = rand() / (float)RAND_MAX * rand() / (float)RAND_MAX;

    for (int len = 0; len < cave_length; len++) {
      // Update cave position using the given angles
      cave_x += sinf(theta) * cosf(phi);
      cave_y += cosf(theta) * cosf(phi);
      cave_z += sinf(phi);

      // Update angles and rate of change
      theta += delta_theta * 0.2f;
      delta_theta = (delta_theta * 0.9f) +
                    (rand() / (float)RAND_MAX - rand() / (float)RAND_MAX);
      phi += delta_phi / 4.0f;
      delta_phi = (delta_phi * 0.75f) +
                  (rand() / (float)RAND_MAX - rand() / (float)RAND_MAX);

      if (rand() / (float)RAND_MAX >= 0.25f) {
        // Generate a random center position
        int center_x = cave_x + (rand() % 4 - 2) * 0.2f;
        int center_y = cave_y + (rand() % 4 - 2) * 0.2f;
        int center_z = cave_z + (rand() % 4 - 2) * 0.2f;

        // Compute the radius based on the height
        float radius = (map->height - center_y) / (float)map->height;
        radius = 1.2f + (radius * 3.5f + 1) * cave_radius;
        radius = radius * sinf(len * M_PI / cave_length);

        fillOblateSpheroid(map, center_x, center_y, center_z, radius,
                           static_cast<uint8_t>(Blocks::AIR_BLOCK));
      }
    }
  }
}

void create_vein(LevelMap* map, float abundance, uint8_t type) {
  int num_veins = (map->length * map->height * map->width * abundance) / 16384;
  for (int i = 0; i < num_veins; i++) {
    int vein_x = rand() % map->length;
    int vein_y = rand() % map->height;
    int vein_z = rand() % map->width;

    // Generate a random cave length
    int veinLength = (rand() / (float)RAND_MAX) * (rand() / (float)RAND_MAX) *
                     75 * abundance;

    // Generate random initial angles and rate of change
    float theta = (rand() / (float)RAND_MAX) * M_PI * 2;
    float delta_theta = 0;
    float phi = (rand() / (float)RAND_MAX) * M_PI * 2;
    float delta_phi = 0;

    for (int len = 0; len < veinLength; len++) {
      // Update cave position using the given angles
      vein_x += sinf(theta) * cosf(phi);
      vein_y += cosf(theta) * cosf(phi);
      vein_z += sinf(phi);

      // Update angles and rate of change
      theta += delta_theta * 0.2f;
      delta_theta = (delta_theta * 0.9f) +
                    (rand() / (float)RAND_MAX - rand() / (float)RAND_MAX);
      phi += delta_phi / 4.0f;
      delta_phi = (delta_phi * 0.9f) +
                  (rand() / (float)RAND_MAX - rand() / (float)RAND_MAX);

      float radius = abundance * sinf(len * M_PI / veinLength) + 1;

      fillOblateSpheroid(map, vein_x, vein_y, vein_z, radius, type);
    }
  }
}

void create_ores(LevelMap* map) {
  create_vein(map, 0.9f, static_cast<uint8_t>(Blocks::COAL_ORE_BLOCK));
  create_vein(map, 0.7f, static_cast<uint8_t>(Blocks::IRON_ORE_BLOCK));
  create_vein(map, 0.5f, static_cast<uint8_t>(Blocks::GOLD_ORE_BLOCK));
  create_vein(map, 0.1f, static_cast<uint8_t>(Blocks::DIAMOND_ORE_BLOCK));
  create_vein(map, 0.25f, static_cast<uint8_t>(Blocks::EMERALD_ORE_BLOCK));
  create_vein(map, 0.35f, static_cast<uint8_t>(Blocks::REDSTONE_ORE_BLOCK));
}

void flood_fill_water(LevelMap* map) {
  // Flood-fill water into the map
  for (int x = 0; x < map->length; x++) {
    for (int z = 0; z < map->width; z++) {
      int y = waterLevel - 1;

      for (; y >= 0; y--) {
        if (GetBlockFromMap(map, x, y, z) ==
            static_cast<uint8_t>(Blocks::AIR_BLOCK))
          SetBlockInMap(map, x, y, z,
                        static_cast<uint8_t>(Blocks::WATER_BLOCK));
        else
          break;
      }
    }
  }

  // Add underground water sources
  int numWaterSources = map->length * map->width / 8000;
  for (int i = 0; i < numWaterSources; i++) {
    // Choose random x and z coordinates
    int x = rand() % map->length;
    int z = rand() % map->width;

    int y = waterLevel - (rand() % 24);

    if (GetBlockFromMap(map, x, y, z) ==
        static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
      SetBlockInMap(map, x, y, z, static_cast<uint8_t>(Blocks::WATER_BLOCK));
    }
  }
}

void flood_fill_lava(LevelMap* map) {
  // Add underground lava sources
  int numLavaSources = map->length * map->width * map->height / 20000;
  for (int i = 0; i < numLavaSources; i++) {
    // Choose random x and z coordinates
    int x = rand() % map->length;
    int y = rand() % map->height - waterLevel;
    int z = rand() % map->width;

    if (y <= 0) continue;

    if (GetBlockFromMap(map, x, y, z) ==
        static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
      SetBlockInMap(map, x, y, z,
                    static_cast<uint8_t>(Blocks::AIR_BLOCK));  // TODO: add lava
    }
  }
}

void create_surface(LevelMap* map, int16_t* heightmap) {
  for (int x = 0; x < map->length; x++) {
    for (int z = 0; z < map->width; z++) {
      bool sandChance = (noise1(x, z) > 8);
      bool gravelChance = (noise2(x, z) > 12);

      int y = heightmap[x + z * map->length];
      if (y >= map->height || y <= 0) continue;

      uint8_t blockAbove = GetBlockFromMap(map, x, y + 1, z);

      if (blockAbove == static_cast<uint8_t>(Blocks::WATER_BLOCK) &&
          gravelChance) {
        SetBlockInMap(
            map, x, y, z,
            static_cast<uint8_t>(Blocks::STONE_BLOCK));  // TODO: Add Gravel
      }

      if (blockAbove == static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
        if (y <= waterLevel && sandChance) {
          SetBlockInMap(map, x, y, z, static_cast<uint8_t>(Blocks::SAND_BLOCK));
        } else {
          while (BoundCheckMap(map, x, y, z) &&
                 GetBlockFromMap(map, x, y, z) ==
                     static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
            y--;
          }

          if (y <= 0 || y > map->height) continue;

          uint8_t blkBelow = GetBlockFromMap(map, x, y, z);
          if (blkBelow != static_cast<uint8_t>(Blocks::STONE_BLOCK) &&
              blkBelow != static_cast<uint8_t>(Blocks::WATER_BLOCK)) {
            y += 1;
            SetBlockInMap(map, x, y, z,
                          static_cast<uint8_t>(Blocks::GRASS_BLOCK));
          }
        }
      }
    }
  }
}

void create_flowers(LevelMap* map, int16_t* heightmap, int off) {
  int numPatches = map->width * map->length / 3000;

  for (int i = 0; i < numPatches; i++) {
    // uint8_t flowerType = (rand() % 2 == 0) ? 37 : 38;
    uint16_t x = rand() % map->length;
    uint16_t z = rand() % map->width;

    for (int j = 0; j < 10; j++) {
      uint16_t fx = x;
      uint16_t fz = z;

      for (int k = 0; k < 5; k++) {
        fx += (rand() % 6) - (rand() % 6);
        fz += (rand() % 6) - (rand() % 6);

        if (BoundCheckMap(map, fx, 0, fz)) {
          uint16_t fy = heightmap[fx + fz * map->length] + 1 + off;

          if (!BoundCheckMap(map, fx, fy, fz)) continue;

          uint8_t blockBelow = GetBlockFromMap(map, fx, fy - 1, fz);

          if (GetBlockFromMap(map, fx, fy, fz) ==
                  static_cast<uint8_t>(Blocks::AIR_BLOCK) &&
              blockBelow == static_cast<uint8_t>(Blocks::GRASS_BLOCK)) {
            // SetBlockInMap(map, fx, fy, fz, flowerType);//TODO: add flower
            // blocks
            SetBlockInMap(map, fx, fy, fz,
                          static_cast<uint8_t>(Blocks::AIR_BLOCK));
          }
        }
      }
    }
  }
}

void create_shrooms(LevelMap* map, int16_t* heightmap) {
  int numPatches = map->width * map->length * map->height / 2000;

  for (int i = 0; i < numPatches; i++) {
    // uint8_t mushType = (rand() % 2 == 0) ? 39 : 40;
    uint16_t x = rand() % map->length;
    uint16_t y = rand() % map->height;
    uint16_t z = rand() % map->width;

    for (int j = 0; j < 20; j++) {
      uint16_t fx = x;
      uint16_t fy = y;
      uint16_t fz = z;

      for (int k = 0; k < 5; k++) {
        fx += (rand() % 6) - (rand() % 6);
        fy += (rand() % 2) - (rand() % 2);
        fz += (rand() % 6) - (rand() % 6);

        if (BoundCheckMap(map, fx, fy, fz) &&
            BoundCheckMap(map, fx, fy - 1, fz) &&
            fy < heightmap[fx + fz * map->length] - 1) {
          uint8_t blockBelow = GetBlockFromMap(map, fx, fy - 1, fz);

          if (GetBlockFromMap(map, fx, fy, fz) ==
                  static_cast<uint8_t>(Blocks::AIR_BLOCK) &&
              blockBelow == static_cast<uint8_t>(Blocks::STONE_BLOCK)) {
            SetBlockInMap(map, fx, fy, fz,
                          static_cast<uint8_t>(Blocks::AIR_BLOCK));
            // SetBlockInMap(map, fx, fy, fz, mushType);//TODO: add mushrooms
            // blocks
          }
        }
      }
    }
  }
}

#include <stdio.h>
bool isSpaceForTree(LevelMap* map, int x, int y, int z, int treeHeight) {
  // Check if the block below is grass
  if (!BoundCheckMap(map, x, y - 1, z) ||
      GetBlockFromMap(map, x, y - 1, z) !=
          static_cast<uint8_t>(Blocks::GRASS_BLOCK)) {
    return false;
  }

  // Check if the trunk region is empty
  for (int j = y + 1; j < y + treeHeight; j++) {
    if (!BoundCheckMap(map, x, j, z) ||
        GetBlockFromMap(map, x, j, z) !=
            static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
      return false;
    }
  }

  // Check if the canopy region is empty
  for (int i = x - 2; i <= x + 2; i++) {
    for (int j = y + treeHeight; j < y + treeHeight + 3; j++) {
      for (int k = z - 2; k <= z + 2; k++) {
        if (!BoundCheckMap(map, i, j, k) ||
            GetBlockFromMap(map, i, j, k) !=
                static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
          return false;
        }
      }
    }
  }

  // If all checks pass, return true
  return true;
}

void growTree(LevelMap* map, int x, int y, int z, int treeHeight) {
  int max = y + treeHeight;
  int m = max;

  for (; m >= y; m--) {
    if (m == max) {
      SetBlockInMap(map, x - 1, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 1, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z - 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z + 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
    } else if (m == max - 1) {
      SetBlockInMap(map, x - 1, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 1, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z - 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z + 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      if (rand() % 2 == 0)
        SetBlockInMap(map, x - 1, m, z - 1,
                      static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      if (rand() % 2 == 0)
        SetBlockInMap(map, x - 1, m, z + 1,
                      static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      if (rand() % 2 == 0)
        SetBlockInMap(map, x + 1, m, z - 1,
                      static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      if (rand() % 2 == 0)
        SetBlockInMap(map, x + 1, m, z + 1,
                      static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      SetBlockInMap(map, x, m, z, static_cast<uint8_t>(Blocks::OAK_LOG_BLOCK));
    } else if (m == max - 2 || m == max - 3) {
      SetBlockInMap(map, x - 1, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 1, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z - 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z + 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      SetBlockInMap(map, x - 1, m, z - 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x - 1, m, z + 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 1, m, z - 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 1, m, z + 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      SetBlockInMap(map, x - 2, m, z - 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x - 2, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x - 2, m, z + 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      SetBlockInMap(map, x + 2, m, z - 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 2, m, z,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 2, m, z + 1,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      SetBlockInMap(map, x - 1, m, z - 2,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z - 2,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 1, m, z - 2,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      SetBlockInMap(map, x - 1, m, z + 2,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x, m, z + 2,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));
      SetBlockInMap(map, x + 1, m, z + 2,
                    static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      if (rand() % 2 == 0)
        SetBlockInMap(map, x - 2, m, z - 2,
                      static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      if (rand() % 2 == 0)
        SetBlockInMap(map, x + 2, m, z - 2,
                      static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      if (rand() % 2 == 0)
        SetBlockInMap(map, x - 2, m, z + 2,
                      static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      if (rand() % 2 == 0)
        SetBlockInMap(map, x + 2, m, z + 2,
                      static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK));

      SetBlockInMap(map, x, m, z, static_cast<uint8_t>(Blocks::OAK_LOG_BLOCK));
    } else {
      SetBlockInMap(map, x, m, z, static_cast<uint8_t>(Blocks::OAK_LOG_BLOCK));
    }
  }
}

void create_trees(LevelMap* map, int16_t* heightmap, int off) {
  int numPatches = (map->width * map->length) / 4000;

  for (int i = 0; i < numPatches; i++) {
    uint16_t x = rand() % map->length;
    uint16_t z = rand() % map->width;

    for (int j = 0; j < 10; j++) {
      uint16_t fx = x;
      uint16_t fz = z;

      for (int k = 0; k < 20; k++) {
        fx += (rand() % 6) - (rand() % 6);
        fz += (rand() % 6) - (rand() % 6);

        if (BoundCheckMap(map, fx, 1, fz)) {
          uint16_t fy = heightmap[fx + fz * map->length] + 1 + off;
          uint16_t th = rand() % 3 + 4;

          if (isSpaceForTree(map, fx, fy, fz, th)) {
            growTree(map, fx, fy, fz, th);
          }
        }
      }
    }
  }
}

void create_plants(LevelMap* map, int16_t* heightmap, int off) {
  TYRA_LOG("Creating flowers...");
  create_flowers(map, heightmap, off);
  TYRA_LOG("Creating shrooms...");
  create_shrooms(map, heightmap);
  TYRA_LOG("Creating trees...");
  create_trees(map, heightmap, off);
}

/**
 * Generate a map
 * https://github.com/UnknownShadow200/ClassiCube/wiki/Minecraft-Classic-map-generation-algorithm
 * @param map
 */
void CrossCraft_WorldGenerator_Generate_Original(LevelMap* map) {
  int16_t* heightMap = new int16_t[map->length * map->width];

  // Generate a heightmap
  TYRA_LOG("Raising...");
  create_heightmap(heightMap, map->length, map->width);

  // Smooth heightmap
  TYRA_LOG("Eroding...");
  smooth_heightmap(heightMap, map->length, map->width);

  // Create Strata
  TYRA_LOG("Soiling...");
  create_strata(map, heightMap);

  // Create Caves
  TYRA_LOG("Carving...");
  create_caves(map);
  create_ores(map);

  // Watering
  TYRA_LOG("Watering...");
  flood_fill_water(map);

  // Melting
  TYRA_LOG("Melting...");
  flood_fill_lava(map);

  // Growing Surface Layer
  TYRA_LOG("Growing...");
  create_surface(map, heightMap);

  // Planting Flora
  TYRA_LOG("Planting...");
  create_plants(map, heightMap, 1);

  delete[] heightMap;
}

void CrossCraft_WorldGenerator_Generate_Island(LevelMap* map) {
  int16_t* heightMap = new int16_t[map->length * map->width];

  // Generate a heightmap
  TYRA_LOG("Raising...");
  create_heightmap(heightMap, map->length, map->width);

  // Smooth heightmap
  TYRA_LOG("Eroding...");
  smooth_heightmap(heightMap, map->length, map->width);

  // Smooth to make an island
  smooth_distance(heightMap, map->length, map->width);

  // Create Strata
  TYRA_LOG("Soiling...");
  create_strata(map, heightMap);

  // Create Caves
  TYRA_LOG("Carving...");
  create_caves(map);
  create_ores(map);

  // Watering
  TYRA_LOG("Watering...");
  flood_fill_water(map);

  // Melting
  TYRA_LOG("Melting...");
  flood_fill_lava(map);

  // Growing Surface Layer
  TYRA_LOG("Growing...");
  create_surface(map, heightMap);

  // Planting Flora
  TYRA_LOG("Planting...");
  create_plants(map, heightMap, 1);

  delete[] heightMap;
}

void CrossCraft_WorldGenerator_Generate_Floating(LevelMap* map) {
  float* densityMap = new float[map->length * map->width * map->height];
  int16_t* heightMap = new int16_t[map->length * map->width];
  int16_t* heightMap2 = new int16_t[map->length * map->width];

  for (int y = 0; y < map->height; y++) {
    for (int z = 0; z < map->width; z++) {
      for (int x = 0; x < map->length; x++) {
        uint32_t index = (y * map->length * map->width) + (z * map->width) + x;

        densityMap[index] = (noise3d(x, y, z) + 1.0f) / 2.0f;

        if (densityMap[index] > 0.67f) {
          SetBlockInMap(map, x, y, z,
                        static_cast<uint8_t>(Blocks::STONE_BLOCK));
        }
      }
    }
  }

  for (int z = 0; z < map->width; z++) {
    for (int x = 0; x < map->length; x++) {
      for (int y = map->height - 1; y >= 0; y--) {
        uint8_t blk = GetBlockFromMap(map, x, y, z);

        if (blk != static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
          heightMap[x + z * map->length] = y;
          break;
        }
      }
    }
  }

  for (int z = 0; z < map->width; z++) {
    for (int x = 0; x < map->length; x++) {
      for (int y = 0; y < map->height; y++) {
        uint8_t blk = GetBlockFromMap(map, x, y, z);

        if (blk != static_cast<uint8_t>(Blocks::AIR_BLOCK)) {
          heightMap2[x + z * map->length] = y;
          break;
        }
      }
    }
  }
  create_strata2(map, heightMap, heightMap2);
  create_surface(map, heightMap);

  create_ores(map);

  create_plants(map, heightMap, 1);

  SetBlockInMap(map, map->spawnX, map->spawnY, map->spawnZ,
                static_cast<uint8_t>(Blocks::BEDROCK_BLOCK));

  delete[] heightMap;
  delete[] heightMap2;
  delete[] densityMap;
}

void CrossCraft_WorldGenerator_Generate_Woods(LevelMap* map) {
  int16_t* heightMap = new int16_t[map->length * map->width];

  // Generate a heightmap
  TYRA_LOG("Raising...");
  create_heightmap(heightMap, map->length, map->width);

  // Smooth heightmap
  TYRA_LOG("Eroding...");
  smooth_heightmap(heightMap, map->length, map->width);

  // Create Strata
  TYRA_LOG("Soiling...");
  create_strata(map, heightMap);

  // Create Caves
  TYRA_LOG("Carving...");
  create_caves(map);
  create_ores(map);

  // Watering
  TYRA_LOG("Watering...");
  flood_fill_water(map);

  // Melting
  TYRA_LOG("Melting...");
  flood_fill_lava(map);

  // Growing Surface Layer
  TYRA_LOG("Growing...");
  create_surface(map, heightMap);

  // Planting Flora
  TYRA_LOG("Planting...");
  create_plants(map, heightMap, 1);
  srand(state.seed + 1);
  create_plants(map, heightMap, 1);

  delete[] heightMap;
}

void CrossCraft_WorldGenerator_Generate_Flat(LevelMap* map) {
  for (uint16_t x = 0; x < map->length; x++) {
    for (uint16_t z = 0; z < map->width; z++) {
      for (int y = 0; y < 64; y++) {
        int block_type = static_cast<uint8_t>(Blocks::AIR_BLOCK);

        if (y == 0) {
          block_type = static_cast<uint8_t>(Blocks::BEDROCK_BLOCK);
        } else if (y == 1) {
          block_type = rand() % 2 == 0
                           ? static_cast<uint8_t>(Blocks::BEDROCK_BLOCK)
                           : static_cast<uint8_t>(Blocks::STONE_BLOCK);
        } else if (y <= 28) {
          block_type = static_cast<uint8_t>(Blocks::STONE_BLOCK);
        } else if (y <= 31) {
          block_type = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
        } else if (y <= 32) {
          block_type = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
        }

        SetBlockInMap(map, x, y, z, block_type);
      }
    }
  }

  int16_t* heightMap = new int16_t[map->length * map->width];

  for (int i = 0; i < map->length * map->width; i++) {
    heightMap[i] = 32;
  }
  create_plants(map, heightMap, 0);
  delete[] heightMap;
}