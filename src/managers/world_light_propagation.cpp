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

  for (size_t ci = 0; ci < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD; ci++) {
    LevelChunk* chunk = pLevel->map.chunks[ci];
    if (chunk == nullptr) continue;
    initSunLight(chunk);
  }
}

void WorldLightPropagation::initSunLight(LevelChunk* chunk) {
  if (chunk == nullptr) return;

  // Assuming it's day-time for world-gen bootstrap
  const u8 initialLevel = 15;

  for (int lx = 0; lx < CHUNK_SIZE; lx++) {
    for (int lz = 0; lz < CHUNK_SIZE; lz++) {
      u8 lv = initialLevel;

      for (int y = pLevel->map.height - 1; y >= 0; y--) {
        const int sy = y / CHUNK_SIZE;
        LevelSection* section = chunk->sections[sy];
        const int ly = y % CHUNK_SIZE;
        const int localIndex =
            ly * CHUNK_SIZE * CHUNK_SIZE + lz * CHUNK_SIZE + lx;

        if (section != nullptr) {
          const auto b = static_cast<Blocks>(section->blocks[localIndex]);

          if (b == Blocks::OAK_LEAVES_BLOCK) {
            lv = (lv >= 1) ? lv - 1 : 0;
          } else if (b == Blocks::WATER_BLOCK) {
            lv = (lv >= 2) ? lv - 2 : 0;
          } else if ((u8)b >= (u8)Blocks::STONE_SLAB &&
                     (u8)b <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
            lv = (lv >= 1) ? lv - 1 : 0;
          } else if (!isTransparent(b)) {
            lv = 0;
          }

          section->lightData[localIndex] =
              (section->lightData[localIndex] & 0x0F) | ((lv & 0x0F) << 4);
        } else {
          // If section is null, it's 100% air-like (or VOID which is transparent)
          // lv remains unchanged (sunlight 15 passes through)
        }

        if (lv > 0) {
          sunlightBfsQueue.emplace(static_cast<uint16_t>(chunk->x + lx),
                                   static_cast<uint16_t>(y),
                                   static_cast<uint16_t>(chunk->z + lz), lv);
        }
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
  // Boundary check is already done by caller, but we must ensure we don't LOAD chunks
  // only to check sunlight. Check if the chunk is actually loaded in Level.
  const uint32_t chunksPerRow = pLevel->map.width / CHUNK_SIZE;
  uint32_t cx = x / CHUNK_SIZE;
  uint32_t cz = z / CHUNK_SIZE;
  if (cx >= chunksPerRow || cz >= (pLevel->map.length / CHUNK_SIZE)) return;
  uint32_t index = (cz * chunksPerRow) + cx;
  if (pLevel->map.chunks[index] == nullptr) return;

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

  for (size_t ci = 0; ci < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD; ci++) {
    LevelChunk* chunk = pLevel->map.chunks[ci];
    if (chunk == nullptr) continue;
    initBlockLight(chunk, blockManager);
  }
}

void WorldLightPropagation::initBlockLight(LevelChunk* chunk,
                                           BlockManager* blockManager) {
  if (chunk == nullptr) return;

  for (int sy = 0; sy < OVERWORLD_V_DISTANCE_IN_CHUNKS; sy++) {
    LevelSection* section = chunk->sections[sy];
    if (section == nullptr) continue;

    for (int ly = 0; ly < CHUNK_SIZE; ly++) {
      const int y = sy * CHUNK_SIZE + ly;
      for (int lz = 0; lz < CHUNK_SIZE; lz++) {
        for (int lx = 0; lx < CHUNK_SIZE; lx++) {
          const int localIndex =
              ly * CHUNK_SIZE * CHUNK_SIZE + lz * CHUNK_SIZE + lx;
          const auto b = static_cast<Blocks>(section->blocks[localIndex]);
          const auto lightValue = blockManager->getBlockLightValue(b);
          if (lightValue > 0) {
            addBlockLight(static_cast<uint16_t>(chunk->x + lx),
                          static_cast<uint16_t>(y),
                          static_cast<uint16_t>(chunk->z + lz), lightValue);
          }
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
  const uint32_t chunksPerRow = pLevel->map.width / CHUNK_SIZE;
  uint32_t cx = x / CHUNK_SIZE;
  uint32_t cz = z / CHUNK_SIZE;
  if (cx >= chunksPerRow || cz >= (pLevel->map.length / CHUNK_SIZE)) return;
  uint32_t index = (cz * chunksPerRow) + cx;
  if (pLevel->map.chunks[index] == nullptr) return;

  auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));
  if (isTransparent(b)) {
    if (pLevel->GetBlockLightFromMap(x, y, z) < nextLightValue) {
      addBlockLight(x, y, z, nextLightValue);
    }
  }
}

// ===============================================================
//  Budget-bounded propagation (Phase 1)
// ===============================================================

bool WorldLightPropagation::updateSunlightBudgeted(u32 budgetCycles) {
  u32 startCycles;
  asm volatile("mfc0 %0, $9" : "=r"(startCycles));
  u32 remainingCycles = budgetCycles;

  // Process removal first, then add (preserves two-phase algorithm)
  if (!sunlightRemovalBfsQueue.empty()) {
    bool removalDone = propagateSunlightRemovalQueueBudgeted(remainingCycles);
    if (!removalDone) return false;  // Budget exhausted
  }

  if (!sunlightBfsQueue.empty()) {
    bool addDone = propagateSunLightAddBFSQueueBudgeted(remainingCycles);
    if (!addDone) return false;  // Budget exhausted
  }

  return true;  // Both queues empty
}

bool WorldLightPropagation::updateBlockLightsBudgeted(u32 budgetCycles) {
  u32 startCycles;
  asm volatile("mfc0 %0, $9" : "=r"(startCycles));
  u32 remainingCycles = budgetCycles;

  // Process removal first, then add
  if (!lightRemovalBfsQueue.empty()) {
    bool removalDone = propagateLightRemovalQueueBudgeted(remainingCycles);
    if (!removalDone) return false;
  }

  if (!lightBfsQueue.empty()) {
    bool addDone = propagateLightAddQueueBudgeted(remainingCycles);
    if (!addDone) return false;
  }

  return true;
}

bool WorldLightPropagation::propagateSunlightRemovalQueueBudgeted(
    u32& remainingCycles) {
  u32 startCycles;
  asm volatile("mfc0 %0, $9" : "=r"(startCycles));

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

    // Check budget every node
    u32 now;
    asm volatile("mfc0 %0, $9" : "=r"(now));
    if ((now - startCycles) >= remainingCycles) {
      remainingCycles = 0;
      return false;  // Budget exhausted, queue not empty
    }
  }

  // Update remaining budget for caller
  u32 endCycles;
  asm volatile("mfc0 %0, $9" : "=r"(endCycles));
  u32 used = (endCycles - startCycles);
  remainingCycles = (used < remainingCycles) ? (remainingCycles - used) : 0;
  return true;  // Queue empty
}

bool WorldLightPropagation::propagateSunLightAddBFSQueueBudgeted(
    u32& remainingCycles) {
  u32 startCycles;
  asm volatile("mfc0 %0, $9" : "=r"(startCycles));

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

    // Check budget
    u32 now;
    asm volatile("mfc0 %0, $9" : "=r"(now));
    if ((now - startCycles) >= remainingCycles) {
      remainingCycles = 0;
      return false;
    }
  }

  u32 endCycles;
  asm volatile("mfc0 %0, $9" : "=r"(endCycles));
  u32 used = (endCycles - startCycles);
  remainingCycles = (used < remainingCycles) ? (remainingCycles - used) : 0;
  return true;
}

bool WorldLightPropagation::propagateLightRemovalQueueBudgeted(
    u32& remainingCycles) {
  u32 startCycles;
  asm volatile("mfc0 %0, $9" : "=r"(startCycles));

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

    u32 now;
    asm volatile("mfc0 %0, $9" : "=r"(now));
    if ((now - startCycles) >= remainingCycles) {
      remainingCycles = 0;
      return false;
    }
  }

  u32 endCycles;
  asm volatile("mfc0 %0, $9" : "=r"(endCycles));
  u32 used = (endCycles - startCycles);
  remainingCycles = (used < remainingCycles) ? (remainingCycles - used) : 0;
  return true;
}

bool WorldLightPropagation::propagateLightAddQueueBudgeted(
    u32& remainingCycles) {
  u32 startCycles;
  asm volatile("mfc0 %0, $9" : "=r"(startCycles));

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

    u32 now;
    asm volatile("mfc0 %0, $9" : "=r"(now));
    if ((now - startCycles) >= remainingCycles) {
      remainingCycles = 0;
      return false;
    }
  }

  u32 endCycles;
  asm volatile("mfc0 %0, $9" : "=r"(endCycles));
  u32 used = (endCycles - startCycles);
  remainingCycles = (used < remainingCycles) ? (remainingCycles - used) : 0;
  return true;
}
