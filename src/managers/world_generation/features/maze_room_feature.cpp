#include "managers/world_generation/features/maze_room_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

MazeRoomFeature::MazeRoomFeature()
    : Feature(false),
      oakTreeFeature(static_cast<uint8_t>(Blocks::OAK_LOG_BLOCK),
                     static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK)),
      birchTreeFeature(static_cast<uint8_t>(Blocks::BIRCH_LOG_BLOCK),
                       static_cast<uint8_t>(Blocks::BIRCH_LEAVES_BLOCK)) {}

bool MazeRoomFeature::place(Level* level, int x, int y, int z) {
  int treeHeight = rand() % 3 + 4;
  const bool isOakTree = rand() % 2 == 0;

  if (oakTreeFeature.isSpaceForTree(level, x, y, z, treeHeight)) {
    if (isOakTree) {
      oakTreeFeature.growTree(level, x, y, z, treeHeight);
    } else {
      birchTreeFeature.growTree(level, x, y, z, treeHeight);
    }

    // Place decorative torches
    level->SetBlockInMap(x - 2, 2, z, static_cast<uint8_t>(Blocks::TORCH));
    level->SetBlockInMap(x + 2, 2, z, static_cast<uint8_t>(Blocks::TORCH));
    level->SetBlockInMap(x, 2, z - 2, static_cast<uint8_t>(Blocks::TORCH));
    level->SetBlockInMap(x, 2, z + 2, static_cast<uint8_t>(Blocks::TORCH));

    level->SetTorchOrientationDataToMap(x - 2, 2, z, BlockOrientation::Top);
    level->SetTorchOrientationDataToMap(x + 2, 2, z, BlockOrientation::Top);
    level->SetTorchOrientationDataToMap(x, 2, z - 2, BlockOrientation::Top);
    level->SetTorchOrientationDataToMap(x, 2, z + 2, BlockOrientation::Top);

    return true;
  }

  return false;
}
