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
  void tick() override;
  void flush() override;
  void waitForAll() override;
  void waitIfTooManyQueuedChunks() override;

 private:
  std::string worldDir;
  
  std::string getRegionFilePath(int x, int z);
  // Internal methods for region file binary format (compressed chunks)
};
