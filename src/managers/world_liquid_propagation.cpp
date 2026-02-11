#include "managers/world_liquid_propagation.hpp"
#include "managers/chunk_manager.hpp"
#include "managers/world_light_propagation.hpp"
#include <tyra>

WorldLiquidPropagation::WorldLiquidPropagation() {}

WorldLiquidPropagation::~WorldLiquidPropagation() {
  affectedChunksIdByLiquidPropagation.clear();
}

void WorldLiquidPropagation::init(Level* level, ChunkManager* chunkManager,
                                  WorldLightPropagation* lightPropagation) {
  pLevel = level;
  pChunkManager = chunkManager;
  pLightPropagation = lightPropagation;
}

// ===============================================================
//  Public API
// ===============================================================

void WorldLiquidPropagation::propagateAll() {
  TYRA_LOG("Propagating liquids...");
  initLiquidExpansion();

  while (waterBfsQueue.empty() == false) propagateWaterAddQueue();
  while (lavaBfsQueue.empty() == false) propagateLavaAddQueue();
}

void WorldLiquidPropagation::updateLiquidWater() {
  propagateWaterRemovalQueue();
  propagateWaterAddQueue();
}

void WorldLiquidPropagation::updateLiquidLava() {
  propagateLavaRemovalQueue();
  propagateLavaAddQueue();
}

void WorldLiquidPropagation::updateChunksAffectedByLiquidPropagation() {
  for (auto chunkPtr : affectedChunksIdByLiquidPropagation) {
    Chunk* moddedChunk = chunkPtr;
    if (moddedChunk) {
      if (moddedChunk->isLoaded()) {
        moddedChunk->rebuild();
      } else {
        moddedChunk->build();
      }
    }
  }

  affectedChunksIdByLiquidPropagation.clear();
}

// ===============================================================
//  Liquid propagation check (called after block removal)
// ===============================================================

void WorldLiquidPropagation::checkLiquidPropagation(uint16_t x, uint16_t y,
                                                    uint16_t z) {
  if (pLevel->BoundCheckMap(x - 1, y, z)) {
    Blocks nl = static_cast<Blocks>(pLevel->GetBlockFromMap(x - 1, y, z));
    u8 level = pLevel->GetLiquidDataFromMap(x - 1, y, z);
    if (nl == Blocks::WATER_BLOCK || nl == Blocks::LAVA_BLOCK)
      addLiquid(x - 1, y, z, (u8)nl, level);
  }

  if (pLevel->BoundCheckMap(x + 1, y, z)) {
    Blocks nr = static_cast<Blocks>(pLevel->GetBlockFromMap(x + 1, y, z));
    u8 level = pLevel->GetLiquidDataFromMap(x + 1, y, z);
    if (nr == Blocks::WATER_BLOCK || nr == Blocks::LAVA_BLOCK)
      addLiquid(x + 1, y, z, (u8)nr, level);
  }

  if (pLevel->BoundCheckMap(x, y - 1, z)) {
    Blocks nd = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y - 1, z));
    u8 level = pLevel->GetLiquidDataFromMap(x, y - 1, z);
    if (nd == Blocks::WATER_BLOCK || nd == Blocks::LAVA_BLOCK)
      addLiquid(x, y - 1, z, (u8)nd, level);
  }

  if (pLevel->BoundCheckMap(x, y, z + 1)) {
    Blocks nf = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z + 1));
    u8 level = pLevel->GetLiquidDataFromMap(x, y, z + 1);
    if (nf == Blocks::WATER_BLOCK || nf == Blocks::LAVA_BLOCK)
      addLiquid(x, y, z + 1, (u8)nf, level);
  }

  if (pLevel->BoundCheckMap(x, y, z - 1)) {
    Blocks nb = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z - 1));
    u8 level = pLevel->GetLiquidDataFromMap(x, y, z - 1);
    if (nb == Blocks::WATER_BLOCK || nb == Blocks::LAVA_BLOCK)
      addLiquid(x, y, z - 1, (u8)nb, level);
  }
}

// ===============================================================
//  Add / Remove liquid
// ===============================================================

void WorldLiquidPropagation::addLiquid(uint16_t x, uint16_t y, uint16_t z,
                                       u8 type, u8 level, u8 orientation) {
  if (level > (u8)LiquidLevel::Percent0) {
    if (type == (u8)Blocks::WATER_BLOCK) {
      waterBfsQueue.emplace(x, y, z, level);
    } else if (type == (u8)Blocks::LAVA_BLOCK) {
      lavaBfsQueue.emplace(x, y, z, level);
      pLightPropagation->addBlockLight(x, y, z, 15);
      pLightPropagation->updateBlockLights();
    }

    pLevel->SetBlockInMap(x, y, z, type);
    pLevel->SetLiquidDataToMap(x, y, z, level);

    const auto prevDir = pLevel->GetLiquidOrientationDataFromMap(x, y, z);

    // Fix traversal orientation to linear
    if (((u8)LiquidOrientation::NorthEast == orientation &&
         LiquidOrientation::NorthWest == prevDir) ||
        ((u8)LiquidOrientation::NorthWest == orientation &&
         LiquidOrientation::NorthEast == prevDir)) {
      pLevel->SetLiquidOrientationDataToMap(x, y, z, LiquidOrientation::North);
    } else if (((u8)LiquidOrientation::NorthEast == orientation &&
                LiquidOrientation::SouthEast == prevDir) ||
               ((u8)LiquidOrientation::SouthEast == orientation &&
                LiquidOrientation::NorthEast == prevDir)) {
      pLevel->SetLiquidOrientationDataToMap(x, y, z, LiquidOrientation::East);
    } else if (((u8)LiquidOrientation::SouthEast == orientation &&
                LiquidOrientation::SouthWest == prevDir) ||
               ((u8)LiquidOrientation::SouthWest == orientation &&
                LiquidOrientation::SouthEast == prevDir)) {
      pLevel->SetLiquidOrientationDataToMap(x, y, z, LiquidOrientation::South);
    } else if (((u8)LiquidOrientation::NorthWest == orientation &&
                LiquidOrientation::SouthWest == prevDir) ||
               ((u8)LiquidOrientation::SouthWest == orientation &&
                LiquidOrientation::NorthWest == prevDir)) {
      pLevel->SetLiquidOrientationDataToMap(x, y, z, LiquidOrientation::West);
    } else {
      pLevel->SetLiquidOrientationDataToMap(
          x, y, z, static_cast<LiquidOrientation>(orientation));
    }

    Chunk* moddedChunk =
        pChunkManager->getChunkByBlockOffset(Vec4(x, y, z));
    if (moddedChunk && moddedChunk->isLoaded()) {
      affectedChunksIdByLiquidPropagation.insert(moddedChunk);
    }
  }
}

void WorldLiquidPropagation::addLiquid(uint16_t x, uint16_t y, uint16_t z,
                                       u8 type, u8 level) {
  addLiquid(x, y, z, type, level, (u8)LiquidOrientation::East);
}

void WorldLiquidPropagation::removeLiquid(uint16_t x, uint16_t y, uint16_t z,
                                          u8 type) {
  u8 liquidLevel = pLevel->GetLiquidDataFromMap(x, y, z);
  removeLiquid(x, y, z, type, liquidLevel);
}

void WorldLiquidPropagation::removeLiquid(uint16_t x, uint16_t y, uint16_t z,
                                          u8 type, u8 level) {
  if (level > (u8)LiquidLevel::Percent0) {
    if (type == (u8)Blocks::WATER_BLOCK) {
      waterRemovalBfsQueue.emplace(x, y, z, level);
    } else if (type == (u8)Blocks::LAVA_BLOCK) {
      lavaRemovalBfsQueue.emplace(x, y, z, level);
      pLightPropagation->removeLight(x, y, z);
      pLightPropagation->updateBlockLights();
    }

    pLevel->SetLiquidDataToMap(x, y, z, level);
  } else {
    pLevel->SetBlockInMap(x, y, z, (u8)Blocks::AIR_BLOCK);
    pLevel->SetLiquidDataToMap(x, y, z, (u8)LiquidLevel::Percent0);
  }

  Chunk* moddedChunk =
      pChunkManager->getChunkByBlockOffset(Vec4(x, y, z));
  if (moddedChunk) {
    affectedChunksIdByLiquidPropagation.insert(moddedChunk);
  }
}

// ===============================================================
//  Init (scan full map for liquid neighbors)
// ===============================================================

void WorldLiquidPropagation::initLiquidExpansion() {
  TYRA_LOG("Initiating water propagation...");

  for (int x = 0; x < pLevel->map.length; x++) {
    for (int z = 0; z < pLevel->map.width; z++) {
      for (int y = pLevel->map.height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));

        if (b == Blocks::AIR_BLOCK) {
          auto liquidValue = LiquidLevel::Percent100;

          if (pLevel->BoundCheckMap(x - 1, y, z)) {
            auto type = pLevel->GetBlockFromMap(x - 1, y, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x - 1, y, z, type, liquidValue);
          } else if (pLevel->BoundCheckMap(x + 1, y, z)) {
            auto type = pLevel->GetBlockFromMap(x + 1, y, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x + 1, y, z, type, liquidValue);
          } else if (pLevel->BoundCheckMap(x, y - 1, z)) {
            auto type = pLevel->GetBlockFromMap(x, y - 1, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x, y - 1, z, type, liquidValue);
          } else if (pLevel->BoundCheckMap(x, y, z - 1)) {
            auto type = pLevel->GetBlockFromMap(x, y, z - 1);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x, y, z - 1, type, liquidValue);
          } else if (pLevel->BoundCheckMap(x, y, z + 1)) {
            auto type = pLevel->GetBlockFromMap(x, y, z + 1);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x, y, z + 1, type, liquidValue);
          }
        }
      }
    }
  }
}

// ===============================================================
//  Water BFS
// ===============================================================

void WorldLiquidPropagation::propagateWaterRemovalQueue() {
  if (waterRemovalBfsQueue.empty() == false) {
    const auto countToUpdateThisTick = waterRemovalBfsQueue.size();
    for (size_t i = 0; i < countToUpdateThisTick; i++) {
      BfsNode liquidNode = waterRemovalBfsQueue.front();

      uint16_t nx = liquidNode.x;
      uint16_t ny = liquidNode.y;
      uint16_t nz = liquidNode.z;
      uint8_t liquidValue = liquidNode.val - 1;

      waterRemovalBfsQueue.pop();

      if (pLevel->BoundCheckMap(nx + 1, ny, nz) &&
          pLevel->GetBlockFromMap(nx + 1, ny, nz) == (u8)Blocks::WATER_BLOCK)
        floodFillLiquidRemove(nx + 1, ny, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);

      if (pLevel->BoundCheckMap(nx, ny, nz + 1) &&
          pLevel->GetBlockFromMap(nx, ny, nz + 1) == (u8)Blocks::WATER_BLOCK)
        floodFillLiquidRemove(nx, ny, nz + 1, (u8)Blocks::WATER_BLOCK,
                              liquidValue);

      if (pLevel->BoundCheckMap(nx - 1, ny, nz) &&
          pLevel->GetBlockFromMap(nx - 1, ny, nz) == (u8)Blocks::WATER_BLOCK)
        floodFillLiquidRemove(nx - 1, ny, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);

      if (pLevel->BoundCheckMap(nx, ny - 1, nz) &&
          pLevel->GetBlockFromMap(nx, ny - 1, nz) == (u8)Blocks::WATER_BLOCK)
        floodFillLiquidRemove(nx, ny - 1, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);

      if (pLevel->BoundCheckMap(nx, ny, nz - 1) &&
          pLevel->GetBlockFromMap(nx, ny, nz - 1) == (u8)Blocks::WATER_BLOCK)
        floodFillLiquidRemove(nx, ny, nz - 1, (u8)Blocks::WATER_BLOCK,
                              liquidValue);

      if (liquidValue > (u8)LiquidLevel::Percent0) {
        waterRemovalBfsQueue.emplace(nx, ny, nz, liquidValue);
      }
    }
  }
}

void WorldLiquidPropagation::propagateWaterAddQueue() {
  if (waterBfsQueue.empty() == false) {
    const auto countToUpdateThisTick = waterBfsQueue.size();

    for (size_t i = 0; i < countToUpdateThisTick; i++) {
      auto liquidNode = waterBfsQueue.front();

      uint16_t nx = liquidNode.x;
      uint16_t ny = liquidNode.y;
      uint16_t nz = liquidNode.z;

      waterBfsQueue.pop();

      s16 nextLevel = liquidNode.val - 1;
      u8 type = static_cast<u8>(Blocks::WATER_BLOCK);

      if (canPropagateLiquid(nx, ny - 1, nz)) {
        floodFillLiquidAdd(nx, ny - 1, nz, type, LiquidLevel::Percent100,
                           (u8)LiquidOrientation::East);
        return;
      }

      if (nextLevel <= (u8)LiquidLevel::Percent0) return;

      if (canPropagateLiquid(nx + 1, ny, nz))
        floodFillLiquidAdd(nx + 1, ny, nz, type, nextLevel,
                           (u8)LiquidOrientation::North);
      if (canPropagateLiquid(nx, ny, nz + 1))
        floodFillLiquidAdd(nx, ny, nz + 1, type, nextLevel,
                           (u8)LiquidOrientation::East);
      if (canPropagateLiquid(nx - 1, ny, nz))
        floodFillLiquidAdd(nx - 1, ny, nz, type, nextLevel,
                           (u8)LiquidOrientation::South);
      if (canPropagateLiquid(nx, ny, nz - 1))
        floodFillLiquidAdd(nx, ny, nz - 1, type, nextLevel,
                           (u8)LiquidOrientation::West);

      if (canPropagateLiquid(nx + 1, ny, nz + 1))
        floodFillLiquidAdd(nx + 1, ny, nz + 1, type, nextLevel - 1,
                           (u8)LiquidOrientation::NorthEast);
      if (canPropagateLiquid(nx + 1, ny, nz - 1))
        floodFillLiquidAdd(nx + 1, ny, nz - 1, type, nextLevel - 1,
                           (u8)LiquidOrientation::NorthWest);
      if (canPropagateLiquid(nx - 1, ny, nz + 1))
        floodFillLiquidAdd(nx - 1, ny, nz + 1, type, nextLevel - 1,
                           (u8)LiquidOrientation::SouthEast);
      if (canPropagateLiquid(nx - 1, ny, nz - 1))
        floodFillLiquidAdd(nx - 1, ny, nz - 1, type, nextLevel - 1,
                           (u8)LiquidOrientation::SouthWest);
    }
  }
}

// ===============================================================
//  Lava BFS
// ===============================================================

void WorldLiquidPropagation::propagateLavaRemovalQueue() {
  if (lavaRemovalBfsQueue.empty() == false) {
    const auto countToUpdateThisTick = lavaRemovalBfsQueue.size();
    for (size_t i = 0; i < countToUpdateThisTick; i++) {
      BfsNode liquidNode = lavaRemovalBfsQueue.front();

      uint16_t nx = liquidNode.x;
      uint16_t ny = liquidNode.y;
      uint16_t nz = liquidNode.z;

      s8 nextLevel = (u8)LiquidLevel::Percent0;
      if (liquidNode.val == (u8)LiquidLevel::Percent100)
        nextLevel = (u8)LiquidLevel::Percent75;
      else if (liquidNode.val == (u8)LiquidLevel::Percent75)
        nextLevel = (u8)LiquidLevel::Percent50;
      else if (liquidNode.val == (u8)LiquidLevel::Percent50)
        nextLevel = (u8)LiquidLevel::Percent25;
      else
        return;

      lavaRemovalBfsQueue.pop();

      if (pLevel->BoundCheckMap(nx + 1, ny, nz) &&
          pLevel->GetBlockFromMap(nx + 1, ny, nz) == (u8)Blocks::LAVA_BLOCK)
        floodFillLiquidRemove(nx + 1, ny, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);

      if (pLevel->BoundCheckMap(nx, ny, nz + 1) &&
          pLevel->GetBlockFromMap(nx, ny, nz + 1) == (u8)Blocks::LAVA_BLOCK)
        floodFillLiquidRemove(nx, ny, nz + 1, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);

      if (pLevel->BoundCheckMap(nx - 1, ny, nz) &&
          pLevel->GetBlockFromMap(nx - 1, ny, nz) == (u8)Blocks::LAVA_BLOCK)
        floodFillLiquidRemove(nx - 1, ny, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);

      if (pLevel->BoundCheckMap(nx, ny - 1, nz) &&
          pLevel->GetBlockFromMap(nx, ny - 1, nz) == (u8)Blocks::LAVA_BLOCK)
        floodFillLiquidRemove(nx, ny - 1, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);

      if (pLevel->BoundCheckMap(nx, ny, nz - 1) &&
          pLevel->GetBlockFromMap(nx, ny, nz - 1) == (u8)Blocks::LAVA_BLOCK)
        floodFillLiquidRemove(nx, ny, nz - 1, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);

      if (nextLevel > (u8)LiquidLevel::Percent0) {
        lavaRemovalBfsQueue.emplace(nx, ny, nz, nextLevel);
      }
    }
  }
}

void WorldLiquidPropagation::propagateLavaAddQueue() {
  if (lavaBfsQueue.empty() == false) {
    const auto countToUpdateThisTick = lavaBfsQueue.size();
    for (size_t i = 0; i < countToUpdateThisTick; i++) {
      auto liquidNode = lavaBfsQueue.front();

      uint16_t nx = liquidNode.x;
      uint16_t ny = liquidNode.y;
      uint16_t nz = liquidNode.z;

      lavaBfsQueue.pop();

      s8 nextLevel = getNextLavaLevel(liquidNode.val);
      u8 type = (u8)Blocks::LAVA_BLOCK;

      if (canPropagateLiquid(nx, ny - 1, nz)) {
        floodFillLiquidAdd(nx, ny - 1, nz, type, LiquidLevel::Percent100,
                           (u8)BlockOrientation::East);
        return;
      }

      if (nextLevel <= (u8)LiquidLevel::Percent0) return;

      if (canPropagateLiquid(nx + 1, ny, nz))
        floodFillLiquidAdd(nx + 1, ny, nz, type, nextLevel,
                           (u8)BlockOrientation::North);
      if (canPropagateLiquid(nx, ny, nz + 1))
        floodFillLiquidAdd(nx, ny, nz + 1, type, nextLevel,
                           (u8)BlockOrientation::East);
      if (canPropagateLiquid(nx - 1, ny, nz))
        floodFillLiquidAdd(nx - 1, ny, nz, type, nextLevel,
                           (u8)BlockOrientation::South);
      if (canPropagateLiquid(nx, ny, nz - 1))
        floodFillLiquidAdd(nx, ny, nz - 1, type, nextLevel,
                           (u8)BlockOrientation::West);

      if (canPropagateLiquid(nx + 1, ny, nz + 1))
        floodFillLiquidAdd(nx + 1, ny, nz + 1, type,
                           getNextLavaLevel(nextLevel),
                           (u8)LiquidOrientation::NorthEast);
      if (canPropagateLiquid(nx + 1, ny, nz - 1))
        floodFillLiquidAdd(nx + 1, ny, nz - 1, type,
                           getNextLavaLevel(nextLevel),
                           (u8)LiquidOrientation::NorthWest);
      if (canPropagateLiquid(nx - 1, ny, nz + 1))
        floodFillLiquidAdd(nx - 1, ny, nz + 1, type,
                           getNextLavaLevel(nextLevel),
                           (u8)LiquidOrientation::SouthEast);
      if (canPropagateLiquid(nx - 1, ny, nz - 1))
        floodFillLiquidAdd(nx - 1, ny, nz - 1, type,
                           getNextLavaLevel(nextLevel),
                           (u8)LiquidOrientation::SouthWest);
    }
  }
}

// ===============================================================
//  BFS helpers
// ===============================================================

void WorldLiquidPropagation::floodFillLiquidRemove(uint16_t x, uint16_t y,
                                                   uint16_t z, u8 type,
                                                   u8 level) {
  u8 neighborLevel = pLevel->GetLiquidDataFromMap(x, y, z);
  if (neighborLevel <= level + 1) {
    removeLiquid(x, y, z, type, level);
  } else if (neighborLevel > level) {
    addLiquid(x, y, z, type, neighborLevel);
  }
}

void WorldLiquidPropagation::floodFillLiquidAdd(uint16_t x, uint16_t y,
                                                uint16_t z, u8 type,
                                                u8 nextLevel, u8 orientation) {
  if (pLevel->GetLiquidDataFromMap(x, y, z) + 1 < nextLevel) {
    addLiquid(x, y, z, type, nextLevel, orientation);
  }
}

const s8 WorldLiquidPropagation::getNextLavaLevel(const s8 currentLevel) {
  s8 nextLevel = (u8)LiquidLevel::Percent0;
  if (currentLevel == (u8)LiquidLevel::Percent100)
    nextLevel = (u8)LiquidLevel::Percent75;
  else if (currentLevel == (u8)LiquidLevel::Percent75)
    nextLevel = (u8)LiquidLevel::Percent50;
  else if (currentLevel == (u8)LiquidLevel::Percent50)
    nextLevel = (u8)LiquidLevel::Percent25;
  else if (currentLevel == (u8)LiquidLevel::Percent25)
    nextLevel = (u8)LiquidLevel::Percent0;

  return nextLevel;
}

u8 WorldLiquidPropagation::canPropagateLiquid(uint16_t x, uint16_t y,
                                              uint16_t z) {
  if (!pLevel->BoundCheckMap(x, y, z)) return false;
  const u8 type = pLevel->GetBlockFromMap(x, y, z);
  return type == (u8)Blocks::AIR_BLOCK || type == (u8)Blocks::GRASS ||
         type == (u8)Blocks::POPPY_FLOWER || type == (u8)Blocks::TORCH ||
         type == (u8)Blocks::DANDELION_FLOWER;
}
