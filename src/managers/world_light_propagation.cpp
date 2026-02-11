#include "managers/world_light_propagation.hpp"
#include "managers/block_manager.hpp"
#include <tyra>

WorldLightPropagation::WorldLightPropagation() {}

WorldLightPropagation::~WorldLightPropagation() {}

void WorldLightPropagation::init(Level* level) { pLevel = level; }

// ===============================================================
//  Sunlight
// ===============================================================

void WorldLightPropagation::initSunLight(uint32_t tick) {
  TYRA_LOG("Initiating SunLight...");

  for (int x = 0; x < pLevel->map.length; x++) {
    for (int z = 0; z < pLevel->map.width; z++) {
      u8 lv = 4;
      auto isDay = tick >= 0 && tick <= 12000;
      if (isDay) {
        lv = 15;
      }

      for (int y = pLevel->map.height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));

        if (b == Blocks::OAK_LEAVES_BLOCK) {
          if (lv >= 1)
            lv -= 1;
          else
            lv = 0;
        } else if (b == Blocks::WATER_BLOCK) {
          if (lv >= 2)
            lv -= 2;
          else
            lv = 0;
        } else if ((u8)b >= (u8)Blocks::STONE_SLAB &&
                   (u8)b <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
          if (lv >= 1)
            lv -= 1;
          else
            lv = 0;
        } else if (b != Blocks::AIR_BLOCK && b != Blocks::GLASS_BLOCK &&
                   b != Blocks::POPPY_FLOWER && b != Blocks::DANDELION_FLOWER &&
                   b != Blocks::GRASS) {
          lv = 0;
        }

        pLevel->SetSunLightInMap(x, y, z, lv);
        sunlightBfsQueue.emplace(x, y, z, lv);
      }
    }
  }
}

void WorldLightPropagation::addSunLight(uint16_t x, uint16_t y, uint16_t z) {
  auto lightLevel = pLevel->GetSunLightFromMap(x, y, z);
  addSunLight(x, y, z, lightLevel);
}

void WorldLightPropagation::addSunLight(uint16_t x, uint16_t y, uint16_t z,
                                        u8 lightLevel) {
  if (lightLevel >= 0) {
    pLevel->SetSunLightInMap(x, y, z, lightLevel);
    sunlightBfsQueue.emplace(x, y, z, lightLevel);
  }
}

void WorldLightPropagation::removeSunLight(uint16_t x, uint16_t y,
                                           uint16_t z) {
  auto lightLevel = pLevel->GetSunLightFromMap(x, y, z);
  removeSunLight(x, y, z, lightLevel);
}

void WorldLightPropagation::removeSunLight(uint16_t x, uint16_t y, uint16_t z,
                                           u8 lightLevel) {
  if (lightLevel > 0) {
    sunlightRemovalBfsQueue.emplace(x, y, z, lightLevel);
    pLevel->SetSunLightInMap(x, y, z, 0);
  }
}

void WorldLightPropagation::updateSunlight() {
  if (sunlightRemovalBfsQueue.empty() == false) {
    propagateSunlightRemovalQueue();
  }

  if (sunlightBfsQueue.empty() == false) {
    propagateSunLightAddBFSQueue();
  }
}

void WorldLightPropagation::checkSunLightAt(uint16_t x, uint16_t y,
                                            uint16_t z) {
  removeSunLight(x + 1, y, z);
  removeSunLight(x - 1, y, z);
  removeSunLight(x, y + 1, z);
  removeSunLight(x, y - 1, z);
  removeSunLight(x, y, z + 1);
  removeSunLight(x, y, z - 1);
  removeSunLight(x, y, z);
}

void WorldLightPropagation::propagateSunLightAddBFSQueue() {
  while (!sunlightBfsQueue.empty()) {
    auto lightNode = sunlightBfsQueue.front();

    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    sunlightBfsQueue.pop();

    int nextLightValue = lightValue - 1;
    if (nextLightValue < 0) continue;

    if (pLevel->BoundCheckMap(nx + 1, ny, nz))
      floodFillSunlightAdd(nx + 1, ny, nz, nextLightValue);
    if (pLevel->BoundCheckMap(nx, ny + 1, nz))
      floodFillSunlightAdd(nx, ny + 1, nz, nextLightValue);
    if (pLevel->BoundCheckMap(nx, ny, nz + 1))
      floodFillSunlightAdd(nx, ny, nz + 1, nextLightValue);
    if (pLevel->BoundCheckMap(nx - 1, ny, nz))
      floodFillSunlightAdd(nx - 1, ny, nz, nextLightValue);
    if (pLevel->BoundCheckMap(nx, ny - 1, nz))
      floodFillSunlightAdd(nx, ny - 1, nz, nextLightValue);
    if (pLevel->BoundCheckMap(nx, ny, nz - 1))
      floodFillSunlightAdd(nx, ny, nz - 1, nextLightValue);
  }
}

void WorldLightPropagation::floodFillSunlightAdd(uint16_t x, uint16_t y,
                                                  uint16_t z,
                                                  u8 nextLightValue) {
  auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));
  if (isTransparent(b)) {
    if (pLevel->GetSunLightFromMap(x, y, z) + 1 < nextLightValue) {
      addSunLight(x, y, z, nextLightValue);
    }
  }
}

void WorldLightPropagation::propagateSunlightRemovalQueue() {
  while (!sunlightRemovalBfsQueue.empty()) {
    BfsNode lightNode = sunlightRemovalBfsQueue.front();

    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    sunlightRemovalBfsQueue.pop();

    if (pLevel->BoundCheckMap(nx + 1, ny, nz))
      floodFillSunlightRemove(nx + 1, ny, nz, lightValue);
    if (pLevel->BoundCheckMap(nx, ny + 1, nz))
      floodFillSunlightRemove(nx, ny + 1, nz, lightValue);
    if (pLevel->BoundCheckMap(nx, ny, nz + 1))
      floodFillSunlightRemove(nx, ny, nz + 1, lightValue);
    if (pLevel->BoundCheckMap(nx - 1, ny, nz))
      floodFillSunlightRemove(nx - 1, ny, nz, lightValue);
    if (pLevel->BoundCheckMap(nx, ny - 1, nz))
      floodFillSunlightRemove(nx, ny - 1, nz, lightValue);
    if (pLevel->BoundCheckMap(nx, ny, nz - 1))
      floodFillSunlightRemove(nx, ny, nz - 1, lightValue);
  }
}

void WorldLightPropagation::floodFillSunlightRemove(uint16_t x, uint16_t y,
                                                    uint16_t z,
                                                    u8 lightLevel) {
  auto neighborLevel = pLevel->GetSunLightFromMap(x, y, z);
  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    removeSunLight(x, y, z);
  } else if (neighborLevel >= lightLevel) {
    addSunLight(x, y, z, neighborLevel);
  }
}

// ===============================================================
//  Block Light
// ===============================================================

void WorldLightPropagation::initBlockLight(BlockManager* blockManager) {
  TYRA_LOG("Initiating block Lights...");

  for (int x = 0; x < pLevel->map.length; x++) {
    for (int z = 0; z < pLevel->map.width; z++) {
      for (int y = pLevel->map.height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y, z));
        auto lightValue = blockManager->getBlockLightValue(b);
        if (lightValue > 0) {
          addBlockLight(x, y, z, lightValue);
        }
      }
    }
  }
}

void WorldLightPropagation::addBlockLight(uint16_t x, uint16_t y, uint16_t z,
                                          u8 lightLevel) {
  if (lightLevel > 0) {
    lightBfsQueue.emplace(x, y, z, lightLevel);
    pLevel->SetBlockLightInMap(x, y, z, lightLevel);
  }
}

void WorldLightPropagation::removeLight(uint16_t x, uint16_t y, uint16_t z) {
  u8 lightLevel = pLevel->GetBlockLightFromMap(x, y, z);
  removeLight(x, y, z, lightLevel);
}

void WorldLightPropagation::removeLight(uint16_t x, uint16_t y, uint16_t z,
                                        u8 lightLevel) {
  lightRemovalBfsQueue.emplace(x, y, z, lightLevel);
  pLevel->SetBlockLightInMap(x, y, z, 0);
}

void WorldLightPropagation::updateBlockLights() {
  if (lightRemovalBfsQueue.empty() == false) {
    propagateLightRemovalQueue();
  }

  if (lightBfsQueue.empty() == false) {
    propagateLightAddQueue();
  }
}

void WorldLightPropagation::propagateLightRemovalQueue() {
  while (lightRemovalBfsQueue.empty() == false) {
    BfsNode lightNode = lightRemovalBfsQueue.front();

    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    lightRemovalBfsQueue.pop();

    if (pLevel->BoundCheckMap(nx + 1, ny, nz))
      floodFillLightRemove(nx + 1, ny, nz, lightValue);
    if (pLevel->BoundCheckMap(nx, ny + 1, nz))
      floodFillLightRemove(nx, ny + 1, nz, lightValue);
    if (pLevel->BoundCheckMap(nx, ny, nz + 1))
      floodFillLightRemove(nx, ny, nz + 1, lightValue);
    if (pLevel->BoundCheckMap(nx - 1, ny, nz))
      floodFillLightRemove(nx - 1, ny, nz, lightValue);
    if (pLevel->BoundCheckMap(nx, ny - 1, nz))
      floodFillLightRemove(nx, ny - 1, nz, lightValue);
    if (pLevel->BoundCheckMap(nx, ny, nz - 1))
      floodFillLightRemove(nx, ny, nz - 1, lightValue);
  }
}

void WorldLightPropagation::floodFillLightRemove(uint16_t x, uint16_t y,
                                                 uint16_t z, u8 lightLevel) {
  auto neighborLevel = pLevel->GetBlockLightFromMap(x, y, z);
  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    removeLight(x, y, z);
  } else if (neighborLevel >= lightLevel) {
    addBlockLight(x, y, z, neighborLevel);
  }
}

void WorldLightPropagation::propagateLightAddQueue() {
  while (!lightBfsQueue.empty()) {
    auto lightNode = lightBfsQueue.front();

    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    lightBfsQueue.pop();

    s16 nextLightValue = lightValue - 1;
    if (nextLightValue <= 0) continue;

    if (pLevel->BoundCheckMap(nx + 1, ny, nz))
      floodFillLightAdd(nx + 1, ny, nz, nextLightValue);
    if (pLevel->BoundCheckMap(nx, ny + 1, nz))
      floodFillLightAdd(nx, ny + 1, nz, nextLightValue);
    if (pLevel->BoundCheckMap(nx, ny, nz + 1))
      floodFillLightAdd(nx, ny, nz + 1, nextLightValue);
    if (pLevel->BoundCheckMap(nx - 1, ny, nz))
      floodFillLightAdd(nx - 1, ny, nz, nextLightValue);
    if (pLevel->BoundCheckMap(nx, ny - 1, nz))
      floodFillLightAdd(nx, ny - 1, nz, nextLightValue);
    if (pLevel->BoundCheckMap(nx, ny, nz - 1))
      floodFillLightAdd(nx, ny, nz - 1, nextLightValue);
  }
}

void WorldLightPropagation::floodFillLightAdd(uint16_t x, uint16_t y,
                                              uint16_t z, u8 nextLightValue) {
  auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));
  if (isTransparent(b)) {
    if (pLevel->GetBlockLightFromMap(x, y, z) < nextLightValue) {
      addBlockLight(x, y, z, nextLightValue);
    }
  }
}
