#include "entities/chunk_provider.hpp"
#include <algorithm>

ChunkProvider::ChunkProvider(ChunkDataSource* generator,
               ChunkDataSource* storage)
    : generator(generator), storage(storage) {}

ChunkProvider::~ChunkProvider() {
  if (generator) delete generator;
  if (storage) delete storage;
}

LevelChunk* ChunkProvider::getChunk(int x, int z) {
  // First, try loading from storage
  LevelChunk* chunk = nullptr;
  if (storage) {
    chunk = storage->getChunk(x, z);
  }

  // If not found in storage, generate it
  if (chunk == nullptr && generator) {
    chunk = generator->getChunk(x, z);
  }

  return chunk;
}

void ChunkProvider::getChunkAsync(int x, int z,
                                  std::function<void(LevelChunk*)> callback) {
  if (storage) {
    storage->getChunkAsync(x, z, [this, x, z, callback](LevelChunk* chunk) {
      if (chunk) {
        callback(chunk);
      } else if (generator) {
        generator->getChunkAsync(x, z, callback);
      } else {
        callback(nullptr);
      }
    });
  } else if (generator) {
    generator->getChunkAsync(x, z, callback);
  } else {
    callback(nullptr);
  }
}

void ChunkProvider::saveChunk(LevelChunk* chunk) {
  if (storage && chunk && chunk->isDirty) {
    storage->saveChunk(chunk);
  }
}

void ChunkProvider::saveChunkAsync(LevelChunk* chunk,
                                   std::function<void()> callback) {
  if (storage && chunk && chunk->isDirty) {
    storage->saveChunkAsync(chunk, callback);
  } else {
    if (callback) callback();
  }
}

void ChunkProvider::generateTerrain(int x, int z) {
  if (generator) generator->generateTerrain(x, z);
}

void ChunkProvider::decorate(int x, int z) {
  if (generator) generator->decorate(x, z);
}

void ChunkProvider::tick() {
  if (generator) generator->tick();
  if (storage) storage->tick();
}

void ChunkProvider::flush() {
  if (generator) generator->flush();
  if (storage) storage->flush();
}

void ChunkProvider::waitForAll() {
  if (generator) generator->waitForAll();
  if (storage) storage->waitForAll();
}

void ChunkProvider::waitIfTooManyQueuedChunks() {
  if (generator) generator->waitIfTooManyQueuedChunks();
  if (storage) storage->waitIfTooManyQueuedChunks();
}

void ChunkProvider::unloadFarChunks(int playerX, int playerZ, int radius) {
  // This logic will be implemented as part of the LRU/paging system
  // in the Level class, which holds the 2D array of loaded chunks.
}
