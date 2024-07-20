#include "managers/mazecraft_generator.hpp"
#include "managers/cross_craft_world_generator.hpp"

void Mazecraft_GenerateMap(unsigned int seed, const u8 width, const u8 height,
                           mazegen::Config cfg) {
  const auto level = CrossCraft_World_GetLevelPtr();

  mazegen::PointSet constraints{{1, 1}, {width - 3, height - 3}};

  auto gen = mazegen::Generator();
  gen.set_seed(seed);
  gen.generate(width, height, cfg, constraints);

  if (!gen.get_warnings().empty()) {
    TYRA_WARN(gen.get_warnings().c_str());
  }

  for (uint16_t x = 0; x < level->map.length; x++) {
    for (uint16_t z = 0; z < level->map.width; z++) {
      int region = gen.region_at(x, z);

      const auto isConstraints =
          constraints.find(mazegen::Point{x, z}) != constraints.end();

      for (int y = 0; y < 64; y++) {
        int block_type = static_cast<uint8_t>(Blocks::AIR_BLOCK);

        if (y == 0) {
          block_type = static_cast<uint8_t>(Blocks::BEDROCK_BLOCK);
        } else if (y == 1) {
          if (mazegen::is_hall(region)) {
            block_type = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
          } else if (mazegen::is_room(region)) {
            block_type = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
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
            } else {
              block_type = static_cast<uint8_t>(Blocks::AIR_BLOCK);
            }

          }
          // else if (x == 0 || x == level->map.length - 1 || z == 0 ||
          //            z == level->map.width - 1) {
          //   block_type = static_cast<uint8_t>(Blocks::STONE_BLOCK);
          // }
          else if (region == mazegen::NOTHING_ID) {
            block_type = static_cast<uint8_t>(Blocks::STONE_BLOCK);
          } else if (mazegen::is_door(region) || mazegen::is_hall(region)) {
            block_type = static_cast<uint8_t>(Blocks::AIR_BLOCK);
          } else {
            // for rooms and halls we just print last 2 digits of their ids
            // block_type = static_cast<uint8_t>(Blocks::STONE_BLOCK);
            block_type = static_cast<uint8_t>(Blocks::AIR_BLOCK);
          }
        }

        SetBlockInMap(&level->map, x, y, z, block_type);
      }
    }
  }

  const auto rooms = gen.get_rooms();
  for (uint16_t i = 0; i < rooms.size(); i++) {
    const auto& room = rooms[i];

    const mazegen::Point mid = {
        room.min_point.x +
            static_cast<u8>((room.max_point.x - room.min_point.x) / 2),
        room.min_point.y +
            static_cast<u8>((room.max_point.y - room.min_point.y) / 2),
    };

    uint16_t fx = mid.x;
    uint16_t fz = mid.y;
    uint16_t fy = 2;
    uint16_t th = rand() % 3 + 4;

    const u8 isOakTree = rand() % 2 == 0;

    if (isSpaceForTree(&level->map, fx, fy, fz, th)) {
      if (isOakTree) {
        growOakTree(&level->map, fx, fy, fz, th);
      } else {
        growBirchTree(&level->map, fx, fy, fz, th);
      }

      SetBlockInMap(&level->map, fx - 2, 2, fz,
                    static_cast<uint8_t>(Blocks::TORCH));
      SetBlockInMap(&level->map, fx + 2, 2, fz,
                    static_cast<uint8_t>(Blocks::TORCH));
      SetBlockInMap(&level->map, fx, 2, fz - 2,
                    static_cast<uint8_t>(Blocks::TORCH));
      SetBlockInMap(&level->map, fx, 2, fz + 2,
                    static_cast<uint8_t>(Blocks::TORCH));

      SetTorchOrientationDataToMap(&level->map, fx - 2, 2, fz,
                                   BlockOrientation::Top);
      SetTorchOrientationDataToMap(&level->map, fx + 2, 2, fz,
                                   BlockOrientation::Top);
      SetTorchOrientationDataToMap(&level->map, fx, 2, fz - 2,
                                   BlockOrientation::Top);
      SetTorchOrientationDataToMap(&level->map, fx, 2, fz + 2,
                                   BlockOrientation::Top);
    }
  }
}
