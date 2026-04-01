#pragma once

#include "entities/chunk_provider.hpp"
#include <string>
#include <functional>

/**
 * @brief Implementation of ChunkSource that reads/writes from the PS2 filesystem.
 * Uses a region-based approach to group chunks (e.g., 32x32 chunks per file).
 */
class ChunkStorage : public ChunkDataSource {
 public:
  ChunkStorage(const std::string& worldDir);
  ~ChunkStorage();

  LevelChunk* getChunk(int x, int z) override;
  void getChunkAsync(int x, int z, std::function<void(LevelChunk*)> callback) override;
  void saveChunk(LevelChunk* chunk) override;
  void saveChunkAsync(LevelChunk* chunk, std::function<void()> callback) override;
  void tick() override;
  void flush() override;
  void waitForAll() override;
  void waitIfTooManyQueuedChunks() override;

 private:
  std::string worldDir;

  static const uint32_t REGIONS_PER_AXIS = (OVERWORLD_H_DISTANCE_IN_CHUNKS / 8); // 16/8 = 2
  static const uint32_t CHUNKS_PER_REGION_AXIS = 8;
  static const uint32_t SLOT_SIZE = 32 * 1024;
  static const uint32_t REGION_HEADER_SIZE = 2 * 1024;

  // Pre-allocated IO buffer to avoid heap fragmentation
  static uint8_t* ioTransferBuffer;
  static void allocateIOBuffer();

  struct RegionHeader {
    uint32_t magic;   // 'TCFR'
    uint32_t version; // 2
    uint32_t sizes[CHUNKS_PER_REGION_AXIS * CHUNKS_PER_REGION_AXIS];
    uint32_t padding[446]; // Pad to 2KB
  };

  std::string getRegionFilePath(int x, int z);
  int getChunkIndexInRegion(int x, int z);
  
  LevelChunk* loadChunkFromRegion(int x, int z);
  void saveChunkToRegion(LevelChunk* chunk);
};
