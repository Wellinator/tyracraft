#include "entities/level.hpp"
#include "entities/World.hpp"
#include <cfloat>
#include <cmath>

static inline void markChunkDirty(Level* level, uint16_t x, uint16_t z) {
  LevelChunk* chunk = level->getChunk(x, z);
  if (chunk) chunk->isDirty = true;
}

Level::Level(int seed) : Singleton<Level>() {
  TYRA_LOG("Generating base level template");
  srand(seed);

  map.width = OVERWORLD_H_DISTANCE;
  map.length = OVERWORLD_H_DISTANCE;
  map.height = OVERWORLD_V_DISTANCE;

  // TODO: move to player class
  map.spawnX = OVERWORLD_H_DISTANCE / 2;
  map.spawnY = 59;
  map.spawnZ = OVERWORLD_H_DISTANCE / 2;

  for (size_t i = 0; i < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD; i++) {
    map.chunks[i] = nullptr;
  }
}

Level::~Level() {
  for (size_t i = 0; i < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD; i++) {
    if (map.chunks[i] != nullptr) {
      delete map.chunks[i];
      map.chunks[i] = nullptr;
    }
  }
}

LevelChunk* Level::getChunk(uint16_t x, uint16_t z) {
  uint32_t chunkX = x / CHUNK_SIZE;
  uint32_t chunkZ = z / CHUNK_SIZE;

  const uint32_t chunkWidth = map.width / CHUNK_SIZE;
  const uint32_t chunkLength = map.length / CHUNK_SIZE;
  if (chunkX >= chunkWidth || chunkZ >= chunkLength) return nullptr;

  uint32_t index = (chunkZ * chunkWidth) + chunkX;
  if (map.chunks[index] == nullptr) {
    // Try to load from provider
    World* world = getInstance()->world;
    auto* provider = world ? world->getChunkProvider() : nullptr;
    if (provider) {
      map.chunks[index] = provider->getChunk(chunkX, chunkZ);
    }
    
    // Fallback if provider fails or doesn't exist (Phase 1 behavior)
    if (map.chunks[index] == nullptr) {
        map.chunks[index] = new LevelChunk(chunkX * 8, chunkZ * 8);
    }
  }

  return map.chunks[index];
}

LevelSection* Level::getSection(uint16_t x, uint16_t y, uint16_t z) {
  LevelChunk* chunk = getChunk(x, z);
  if (chunk == nullptr) return nullptr;
  
  uint16_t sy = y / CHUNK_SIZE;
  LevelSection* section = chunk->getSection(sy);
  
  if (section == nullptr) {
    chunk->allocateSection(sy);
    section = chunk->getSection(sy);
  }
  
  return section;
}

// Gets the position in the data array from the given x, y, and z coordinates.
uint32_t Level::GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z) {
  const u32 w = (u32)OVERWORLD_H_DISTANCE;
  const u32 l = (u32)OVERWORLD_H_DISTANCE;
  return x + y * w + z * w * l;
}

uint32_t Level::OffsetToIndex(const Vec4& offset) {
  return (offset.y * map.length * map.width) + (offset.z * map.width) +
         offset.x;
}

// Gets the metadata value at the given coordinates in the map.
uint8_t Level::GetMetaDataFromMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return 0;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  return section->metaData[index];
}

// Sets the metadata value at the given coordinates in the map.
uint8_t Level::SetMetaDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                uint8_t data) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return 0;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  markChunkDirty(this, x, z);
  return section->metaData[index] = data;
}

void Level::SetLiquidOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                          const LiquidOrientation orientation) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);

  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) << 5 & LIQUID_ORIENTATION_MASK;

  section->metaData[index] =
      (section->metaData[index] & ~LIQUID_ORIENTATION_MASK) | newvalue;
  markChunkDirty(this, x, z);
}

void Level::SetTorchOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                         const BlockOrientation orientation) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);

  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) & TORCH_ORIENTATION_MASK;

  section->metaData[index] =
      (section->metaData[index] & ~TORCH_ORIENTATION_MASK) | newvalue;
  markChunkDirty(this, x, z);
}

void Level::SetBlockOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                         const BlockOrientation orientation) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);

  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) & BLOCK_ORIENTATION_MASK;

  section->metaData[index] =
      (section->metaData[index] & ~BLOCK_ORIENTATION_MASK) | newvalue;
  markChunkDirty(this, x, z);
}

void Level::SetSlabOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                        const SlabOrientation orientation) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);

  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) << 2 & SLAB_ORIENTATION_MASK;

  section->metaData[index] =
      (section->metaData[index] & ~SLAB_ORIENTATION_MASK) | newvalue;
  markChunkDirty(this, x, z);
}

void Level::ResetSlabOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);

  section->metaData[index] &= ~SLAB_ORIENTATION_MASK;
  markChunkDirty(this, x, z);
}

LiquidOrientation Level::GetLiquidOrientationDataFromMap(uint16_t x, uint16_t y,
                                                         uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return LiquidOrientation::East;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  const uint8_t response = section->metaData[index] & LIQUID_ORIENTATION_MASK;
  return static_cast<LiquidOrientation>(response >> 5);
}

BlockOrientation Level::GetTorchOrientationDataFromMap(uint16_t x, uint16_t y,
                                                       uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return BlockOrientation::East;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  const uint8_t response = section->metaData[index] & TORCH_ORIENTATION_MASK;
  return static_cast<BlockOrientation>(response);
}

BlockOrientation Level::GetBlockOrientationDataFromMap(uint16_t x, uint16_t y,
                                                       uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return BlockOrientation::East;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  const uint8_t response = section->metaData[index] & BLOCK_ORIENTATION_MASK;
  return static_cast<BlockOrientation>(response);
}

SlabOrientation Level::GetSlabOrientationDataFromMap(uint16_t x, uint16_t y,
                                                     uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return SlabOrientation::Bottom;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  const uint8_t response = section->metaData[index] & SLAB_ORIENTATION_MASK;
  return static_cast<SlabOrientation>(response >> 2);
}

void Level::SetIsUpperHalfDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                    const bool isUpper) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);

  const uint8_t newvalue = (isUpper ? 1 : 0) << 3 & IS_UPPER_HALF_MASK;
  section->metaData[index] =
      (section->metaData[index] & ~IS_UPPER_HALF_MASK) | newvalue;
  markChunkDirty(this, x, z);
}

void Level::ResetIsUpperHalfDataToMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);

  section->metaData[index] &= ~IS_UPPER_HALF_MASK;
  markChunkDirty(this, x, z);
}

bool Level::GetIsUpperHalfDataFromMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return false;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  return (section->metaData[index] & IS_UPPER_HALF_MASK) > 0;
}

// Set the liquid metadata value at the given coordinates in the map.
void Level::SetLiquidDataToMap(uint16_t x, uint16_t y, uint16_t z,
                               const u8 liquidLevel) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);

  const uint8_t newvalue = liquidLevel << 2 & LIQUID_LEVEL_MASK;
  section->metaData[index] =
      (section->metaData[index] & ~LIQUID_LEVEL_MASK) | newvalue;
  markChunkDirty(this, x, z);
}

// Gets the liquid metadata value at the given coordinates in the map.
u8 Level::GetLiquidDataFromMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return 0;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  return (section->metaData[index] & LIQUID_LEVEL_MASK) >> 2;
}

// Gets the light data value at the given coordinates in the map.
uint8_t Level::GetLightDataFromMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return 0;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  return section->lightData[index];
}

// Gets the light value at the given coordinates in the map.
uint8_t Level::GetLightFromMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return 0;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  uint8_t v = section->lightData[index];
  uint8_t res = ((v & 0xF0) >> 4) + (v & 0x0F);

  if (res > 0x0F) return 0x0F;
  return res;
}

// Gets the block ID at the given coordinates in the map.
uint8_t Level::GetBlockFromMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return (uint8_t)Blocks::VOID;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  return section->blocks[index];
}

uint8_t Level::GetBlockFromMap(Vec4* offset) {
  return GetBlockFromMap(static_cast<uint16_t>(offset->x),
                         static_cast<uint16_t>(offset->y),
                         static_cast<uint16_t>(offset->z));
}

uint8_t Level::SafeGetBlockFromMap(uint16_t x, uint16_t y, uint16_t z) {
  return BoundCheckMap(x, y, z) ? GetBlockFromMap(x, y, z)
                                : (uint8_t)Blocks::VOID;
}

// Gets the block ID at the given coordinates in the map.
uint8_t Level::GetBlockFromMapByIndex(uint32_t index) {
  uint16_t x = index % map.width;
  uint16_t z = (index / map.width) % map.length;
  uint16_t y = index / (map.width * map.length);
  return GetBlockFromMap(x, y, z);
}

// Sets the block ID at the given coordinates in the map.
void Level::SetBlockInMap(uint16_t x, uint16_t y, uint16_t z, uint8_t block) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  section->blocks[index] = block;
  markChunkDirty(this, x, z);
}

// Sets the block ID at the given coordinates in the map.
void Level::SetBlockInMapByIndex(uint32_t index, uint8_t block) {
  uint16_t x = index % map.width;
  uint16_t z = (index / map.width) % map.length;
  uint16_t y = index / (map.width * map.length);
  SetBlockInMap(x, y, z, block);
}

uint8_t Level::GetBlockLightFromMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return 0;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  return section->lightData[index] & 0x0F;
}

uint8_t Level::GetSunLightFromMap(uint16_t x, uint16_t y, uint16_t z) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return 0;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  return (section->lightData[index] >> 4) & 0xF;
}

void Level::SetBlockLightInMap(uint16_t x, uint16_t y, uint16_t z,
                               uint16_t light) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  section->lightData[index] = (section->lightData[index] & 0xF0) | (light & 0x0F);
  markChunkDirty(this, x, z);
}

void Level::SetSunLightInMap(uint16_t x, uint16_t y, uint16_t z,
                             uint16_t light) {
  LevelSection* section = getSection(x, y, z);
  if (section == nullptr) return;
  uint16_t index = (y % CHUNK_SIZE) * CHUNK_SIZE * CHUNK_SIZE +
                   (z % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE);
  section->lightData[index] = (section->lightData[index] & 0x0F) | ((light & 0x0F) << 4);
  markChunkDirty(this, x, z);
}

void Level::unloadFarChunks(int playerChunkX, int playerChunkZ, int radius) {
  for (size_t i = 0; i < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD; i++) {
    LevelChunk* chunk = map.chunks[i];
    if (chunk == nullptr) continue;

    int cx = chunk->x / CHUNK_SIZE;
    int cz = chunk->z / CHUNK_SIZE;

    int dx = std::abs(cx - playerChunkX);
    int dz = std::abs(cz - playerChunkZ);

    if (dx > radius || dz > radius) {
      // Chunk is far away, unload it
      auto* provider = world ? world->getChunkProvider() : nullptr;
      LevelChunk* chunkToUnload = map.chunks[i];
      map.chunks[i] = nullptr;

      if (provider && chunkToUnload->isDirty) {
        provider->saveChunkAsync(chunkToUnload,
                                 [chunkToUnload]() { delete chunkToUnload; });
      } else {
        delete chunkToUnload;
      }
    }
  }
}

void Level::unloadChunk(int chunkX, int chunkZ) {
  const uint32_t chunkWidth = map.width / CHUNK_SIZE;
  uint32_t index = (chunkZ * chunkWidth) + chunkX;

  if (index < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD && map.chunks[index] != nullptr) {
    auto* provider = world ? world->getChunkProvider() : nullptr;
    LevelChunk* chunk = map.chunks[index];
    map.chunks[index] = nullptr;

    if (provider && chunk->isDirty) {
      provider->saveChunkAsync(chunk, [chunk]() { delete chunk; });
    } else {
      delete chunk;
    }
  }
}

void Level::preLoadChunk(int x, int z, std::function<void()> onDone) {
  const uint32_t chunkX = x / CHUNK_SIZE;
  const uint32_t chunkZ = z / CHUNK_SIZE;
  const uint32_t chunkWidth = map.width / CHUNK_SIZE;
  const uint32_t chunkLength = map.length / CHUNK_SIZE;

  if (chunkX >= chunkWidth || chunkZ >= chunkLength) {
    if (onDone) onDone();
    return;
  }

  const uint32_t index = (chunkZ * chunkWidth) + chunkX;

  // Already in memory
  if (map.chunks[index] != nullptr) {
    if (onDone) onDone();
    return;
  }

  // To avoid duplicate loading tasks for the same chunk,
  // we could use a set, but let's keep it simple for now as World::scheduleChunks
  // already has its own deduplication via bitsests.
  // However, we still mark it with a temporary placeholder or similar if needed.
  // For now, let's just trigger the async load.

  World* world = getInstance()->world;
  auto* provider = world ? world->getChunkProvider() : nullptr;

  if (provider) {
    provider->getChunkAsync(chunkX, chunkZ, [this, index, onDone](LevelChunk* chunk) {
      if (chunk && map.chunks[index] == nullptr) {
        map.chunks[index] = chunk;
      } else if (chunk) {
        // Someone else loaded it in the meantime? Unlikely on main thread, but safety first.
        delete chunk;
      }
      
      if (onDone) onDone();
    });
  } else {
    // Fallback if no provider
    if (onDone) onDone();
  }
}

void Level::saveAllChunks() {
  auto* provider = world ? world->getChunkProvider() : nullptr;
  if (!provider) return;

  for (size_t i = 0; i < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD; i++) {
    if (map.chunks[i] != nullptr) {
      provider->saveChunk(map.chunks[i]);
      RotateThreadReadyQueue(100);  // Yield to other threads of the same or higher priority
    }
  }
}

void Level::unloadAllChunks() {
  for (size_t i = 0; i < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD; i++) {
    if (map.chunks[i] != nullptr) {
      int cx = map.chunks[i]->x / CHUNK_SIZE;
      int cz = map.chunks[i]->z / CHUNK_SIZE;
      unloadChunk(cx, cz);
    }
  }
}

// Returns true if the given coordinates are within the bounds of the map.
bool Level::BoundCheckMap(uint16_t x, uint16_t y, uint16_t z) {
  return (x < map.length && y < map.height && z < map.width);
}

uint8_t Level::getBlockByWorldPosition(const Vec4* pos) {
  Vec4 offset = worldPosToOffset(*pos);
  return BoundCheckMap(offset.x, offset.y, offset.z)
             ? GetBlockFromMap(offset.x, offset.y, offset.z)
             : static_cast<uint8_t>(Blocks::VOID);
}

Vec4 Level::worldPosToOffset(const Vec4& pos) {
  Vec4 offset = ((pos + BLOCK_SIZE_VEC) / DOUBLE_BLOCK_SIZE_VEC);
  return Vec4(std::floor(offset.x), std::floor(offset.y), std::floor(offset.z));
}

Vec4 Level::worldPosToOffsetNotRounded(const Vec4& pos) {
  Vec4 offset = ((pos + BLOCK_SIZE_VEC) / DOUBLE_BLOCK_SIZE_VEC);
  return Vec4(offset.x, offset.y, offset.z);
}

Vec4 Level::offsetToWorldPos(const Vec4* offset) {
  return (*offset) * DOUBLE_BLOCK_SIZE;
}

Vec4 Level::offsetToWorldPos(const Vec4& offset) {
  return offset * DOUBLE_BLOCK_SIZE;
}

Vec4 Level::roundToBlockCenter(const Vec4& pos) {
  return offsetToWorldPos(worldPosToOffset(pos));
}

void Level::getIntersectedBlocks(
    const Vec4& start, const Vec4& end,
    std::vector<LevelIntersectQueryResult>* pResults) {
  Vec4 startOffset = worldPosToOffsetNotRounded(start);
  Vec4 endOffset = worldPosToOffsetNotRounded(end);

  // Clear results vector
  pResults->clear();

  // Current position
  int x = (int)startOffset.x;
  int y = (int)startOffset.y;
  int z = (int)startOffset.z;

  // Direction and step
  Vec4 delta = endOffset - startOffset;
  int stepX = delta.x > 0 ? 1 : -1;
  int stepY = delta.y > 0 ? 1 : -1;
  int stepZ = delta.z > 0 ? 1 : -1;

  // Calculate t delta values for DDA
  float tDeltaX = delta.x != 0 ? std::abs(1.0f / delta.x) : FLT_MAX;
  float tDeltaY = delta.y != 0 ? std::abs(1.0f / delta.y) : FLT_MAX;
  float tDeltaZ = delta.z != 0 ? std::abs(1.0f / delta.z) : FLT_MAX;

  // Calculate initial t values
  float tMaxX = delta.x != 0 ? (stepX > 0 ? (x + 1 - startOffset.x)
                                          : (startOffset.x - x)) *
                                   tDeltaX
                             : FLT_MAX;
  float tMaxY = delta.y != 0 ? (stepY > 0 ? (y + 1 - startOffset.y)
                                          : (startOffset.y - y)) *
                                   tDeltaY
                             : FLT_MAX;
  float tMaxZ = delta.z != 0 ? (stepZ > 0 ? (z + 1 - startOffset.z)
                                          : (startOffset.z - z)) *
                                   tDeltaZ
                             : FLT_MAX;

  // DDA traversal
  int maxIterations = startOffset.distanceTo(endOffset) * 3;
  int iterations = 0;

  while (iterations < maxIterations) {
    // Check bounds and add current block
    if (x >= 0 && y >= 0 && z >= 0 && x < OVERWORLD_H_DISTANCE &&
        y < OVERWORLD_V_DISTANCE && z < OVERWORLD_H_DISTANCE) {
      // Calculate hit position as the intersection point with the current voxel
      Vec4 blockOffset(x, y, z);
      u8 blockType = GetBlockFromMap(x, y, z);
      if (blockType > (u8)Blocks::AIR_BLOCK) {
        Vec4 hitPos = offsetToWorldPos(blockOffset);
        pResults->emplace_back(
            LevelIntersectQueryResult{blockOffset, hitPos, blockType});
      }
    }

    // Check if we've reached the end
    if (x == (int)endOffset.x && y == (int)endOffset.y && z == (int)endOffset.z)
      break;

    // Move to next voxel
    if (tMaxX < tMaxY && tMaxX < tMaxZ) {
      x += stepX;
      tMaxX += tDeltaX;
    } else if (tMaxY < tMaxZ) {
      y += stepY;
      tMaxY += tDeltaY;
    } else {
      z += stepZ;
      tMaxZ += tDeltaZ;
    }

    iterations++;
  }
}

void Level::getIntersectedBlocksByAABB(
    const Vec4& aabbMin, const Vec4& aabbMax,
    std::vector<LevelIntersectQueryResult>* pResults) {
  // Convert world positions to block offsets
  Vec4 minOffset = worldPosToOffsetNotRounded(aabbMin);
  Vec4 maxOffset = worldPosToOffsetNotRounded(aabbMax);

  // Clear results vector
  pResults->clear();

  // Get the range of blocks to check (floor min, ceil max)
  int startX = std::max(0, (int)std::floor(minOffset.x));
  int startY = std::max(0, (int)std::floor(minOffset.y));
  int startZ = std::max(0, (int)std::floor(minOffset.z));

  int endX = std::min(OVERWORLD_H_DISTANCE - 1, (int)std::ceil(maxOffset.x));
  int endY = std::min(OVERWORLD_V_DISTANCE - 1, (int)std::ceil(maxOffset.y));
  int endZ = std::min(OVERWORLD_H_DISTANCE - 1, (int)std::ceil(maxOffset.z));

  // Iterate through all blocks in the AABB range
  for (int x = startX; x <= endX; x++) {
    for (int y = startY; y <= endY; y++) {
      for (int z = startZ; z <= endZ; z++) {
        // Check if block coordinates are within bounds
        if (x >= 0 && y >= 0 && z >= 0 && x < OVERWORLD_H_DISTANCE &&
            y < OVERWORLD_V_DISTANCE && z < OVERWORLD_H_DISTANCE) {
          u8 blockType = GetBlockFromMap(x, y, z);

          // Only add non-air blocks
          if (blockType > (u8)Blocks::AIR_BLOCK) {
            Vec4 blockOffset(x, y, z);
            Vec4 hitPos = offsetToWorldPos(blockOffset);

            pResults->emplace_back(
                LevelIntersectQueryResult{blockOffset, hitPos, blockType});
          }
        }
      }
    }
  }
}

bool Level::isPositionEmpty(const uint16_t& x, const uint16_t& y,
                            const uint16_t& z) {
  const auto blk = GetBlockFromMap(x, y, z);
  return blk == (uint8_t)Blocks::AIR_BLOCK;
}

bool Level::isGrassAtPosition(const uint16_t& x, const uint16_t& y,
                              const uint16_t& z) {
  const auto blk = GetBlockFromMap(x, y, z);
  return blk == (uint8_t)Blocks::GRASS;
}

bool Level::isReplaceableBySolidBlock(const uint16_t& x, const uint16_t& y,
                                      const uint16_t& z) {
  const auto blk = static_cast<Blocks>(GetBlockFromMap(x, y, z));
  return blk == Blocks::AIR_BLOCK || blk == Blocks::WATER_BLOCK ||
         blk == Blocks::LAVA_BLOCK || blk == Blocks::GRASS;
}