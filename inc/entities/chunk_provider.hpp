#pragma once

#include "entities/level_chunk.hpp"
#include <functional>

/**
 * @brief Interface for getting chunks. 
 * Can be a RandomLevelSource (generation) or a ChunkStorage (loading from file).
 */
class ChunkDataSource {
 public:
  virtual ~ChunkDataSource() {}
  virtual LevelChunk* getChunk(int x, int z) = 0;
  virtual void getChunkAsync(int x, int z, std::function<void(LevelChunk*)> callback) {
    callback(getChunk(x, z));
  }
  virtual void saveChunk(LevelChunk* chunk) = 0;
  virtual void saveChunkAsync(LevelChunk* chunk, std::function<void()> callback) {
    saveChunk(chunk);
    if (callback) callback();
  }
  virtual void generateTerrain(int x, int z) {}
  virtual void carve(int x, int z) {}
  virtual void decorate(int x, int z) {}
  virtual void tick() {}
  virtual void flush() {}
  virtual void waitForAll() {}
  virtual void waitIfTooManyQueuedChunks() {}
};

/**
 * @brief Manages the high-level chunk lifecycle, including caching.
 */
class ChunkProvider : public ChunkDataSource {
 public:
  ChunkProvider(ChunkDataSource* generator, ChunkDataSource* storage);
  ~ChunkProvider();

  LevelChunk* getChunk(int x, int z) override;
  void getChunkAsync(int x, int z, std::function<void(LevelChunk*)> callback) override;
  void saveChunk(LevelChunk* chunk) override;
  void saveChunkAsync(LevelChunk* chunk, std::function<void()> callback) override;
  void generateTerrain(int x, int z) override;
  void carve(int x, int z) override;
  void decorate(int x, int z) override;
  void tick() override;
  void flush() override;
  void waitForAll() override;
  void waitIfTooManyQueuedChunks() override;

  void unloadFarChunks(int playerX, int playerZ, int radius);

 private:
  ChunkDataSource* generator;
  ChunkDataSource* storage;
  
  // Cache of chunks currently in RAM (managed by Level)
  // For now, Level::map.chunks serves as the primary storage.
};
