#include "managers/world_generation/maze_level_source.hpp"
#include "utils.hpp"
#include <algorithm>

namespace TyraCraft {

MazeLevelSource::MazeLevelSource(uint32_t t_seed) : seed(t_seed) {
  // Config logic moved from World::generate
  if (seed < 2) {
    cfg.DEADEND_CHANCE = 0.1;
    cfg.WIGGLE_CHANCE = 0.1;
    width = 16;
    height = 16;
  } else if (seed < 5) {
    cfg.DEADEND_CHANCE = 0.20;
    cfg.WIGGLE_CHANCE = 0.20;
    width = 32;
    height = 32;
  } else if (seed < 10) {
    cfg.DEADEND_CHANCE = 0.3;
    cfg.WIGGLE_CHANCE = 0.3;
    width = 48;
    height = 48;
  } else if (seed < 15) {
    cfg.DEADEND_CHANCE = 0.35f;
    cfg.WIGGLE_CHANCE = 0.35f;
    width = 56;
    height = 56;
  } else if (seed < 20) {
    cfg.DEADEND_CHANCE = 0.4f;
    cfg.WIGGLE_CHANCE = 0.4f;
    width = 64;
    height = 64;
  } else {
    cfg.DEADEND_CHANCE = 0.5;
    cfg.WIGGLE_CHANCE = 0.5;
    width = 128;
    height = 128;
  }

  cfg.CONSTRAIN_HALL_ONLY = false;
  cfg.EXTRA_CONNECTION_CHANCE = 0.12;
  constraints = {{1, 1}, {static_cast<u8>(width - 3), static_cast<u8>(height - 3)}};

  initMazegen();
}

void MazeLevelSource::initMazegen() {
  gen.set_seed(seed);
  gen.generate(width, height, cfg, constraints);

  if (!gen.get_warnings().empty()) {
    TYRA_WARN(gen.get_warnings().c_str());
  }
}

void MazeLevelSource::generateChunk(Level* level, int chunkX, int chunkZ) {
  int startX = chunkX * CHUNK_SIZE;
  int startZ = chunkZ * CHUNK_SIZE;

  for (int x = startX; x < startX + CHUNK_SIZE; x++) {
    for (int z = startZ; z < startZ + CHUNK_SIZE; z++) {
      if (x >= level->map.length || z >= level->map.width) continue;

      int region = gen.region_at(x, z);
      const bool isConstraints = constraints.find(mazegen::Point{static_cast<u8>(x), static_cast<u8>(z)}) != constraints.end();

      for (int y = 0; y < 64; y++) {
        int block_type = static_cast<uint8_t>(Blocks::AIR_BLOCK);

        if (y == 0) {
          block_type = static_cast<uint8_t>(Blocks::BEDROCK_BLOCK);
        } else if (y == 1) {
          if (mazegen::is_hall(region)) {
            block_type = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
          } else {
            block_type = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
          }
        } else if (y > 1 && y < 8) {
          if (isConstraints) {
            if (y == 2) {
              if (x == 1 && z == 1)
                block_type = static_cast<uint8_t>(Blocks::GLOWSTONE_BLOCK);
              else
                block_type = static_cast<uint8_t>(Blocks::JACK_O_LANTERN_BLOCK);
            }
          } else if (region == mazegen::NOTHING_ID) {
            block_type = static_cast<uint8_t>(Blocks::STONE_BLOCK);
          }
        }

        level->SetBlockInMap(x, y, z, block_type);
      }
    }
  }
}

void MazeLevelSource::carve(Level* level, int chunkX, int chunkZ) {
  // Mazes don't currently have carving features like caves/canyons
  (void)level;
  (void)chunkX;
  (void)chunkZ;
}

void MazeLevelSource::postProcess(Level* level, int chunkX, int chunkZ) {
  // Rooms decoration
  const auto rooms = gen.get_rooms();
  int startX = chunkX * CHUNK_SIZE;
  int startZ = chunkZ * CHUNK_SIZE;

  for (const auto& room : rooms) {
    const mazegen::Point mid = {
        static_cast<u8>(room.min_point.x + (room.max_point.x - room.min_point.x) / 2),
        static_cast<u8>(room.min_point.y + (room.max_point.y - room.min_point.y) / 2),
    };

    // Check if the room's midpoint is within this chunk
    if (mid.x >= startX && mid.x < startX + CHUNK_SIZE &&
        mid.y >= startZ && mid.y < startZ + CHUNK_SIZE) {
      roomFeature.place(level, mid.x, 2, mid.y);
    }
  }
}

}  // namespace TyraCraft
