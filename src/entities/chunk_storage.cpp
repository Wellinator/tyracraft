#include "entities/chunk_storage.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#include "managers/save/save_serializer.hpp"
#include <cstdio>
#include "managers/background_task_service.hpp"

static bool ensureDirExists(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    return S_ISDIR(st.st_mode);
  }

  return mkdir(path.c_str(), 0777) == 0;
}

ChunkStorage::ChunkStorage(const std::string& worldDir) : worldDir(worldDir) {
  // Ensure world/chunks directories exist
  ensureDirExists(worldDir);
  std::string chunksPath = worldDir + "/chunks";
  ensureDirExists(chunksPath);
}

ChunkStorage::~ChunkStorage() {}

LevelChunk* ChunkStorage::getChunk(int x, int z) {
  char filename[256];
  snprintf(filename, sizeof(filename), "%s/chunks/c.%d.%d.tc", worldDir.c_str(), x, z);

  // For new worlds most chunks won't exist yet; skip gzopen to avoid noisy
  // host filesystem error logs for missing files.
  struct stat st;
  if (stat(filename, &st) != 0) return nullptr;
  
  gzFile file = gzopen(filename, "rb");
  if (!file) return nullptr;

  TyraCraft::SaveSerializer serializer(file);
  LevelChunk* chunk = new LevelChunk(x * CHUNK_SIZE, z * CHUNK_SIZE);

  // Read header mask
  uint8_t mask;
  if (!serializer.ReadUInt8(&mask)) {
    gzclose(file);
    delete chunk;
    return nullptr;
  }

  // Read sections
  for (int i = 0; i < OVERWORLD_V_DISTANCE_IN_CHUNKS; i++) {
    if (mask & (1 << i)) {
      chunk->allocateSection(i);
      LevelSection* section = chunk->getSection(i);
      serializer.ReadBuffer(section->blocks, CHUNK_LENGTH);
      serializer.ReadBuffer(section->lightData, CHUNK_LENGTH);
      serializer.ReadBuffer(section->metaData, CHUNK_LENGTH);
    }
  }

  gzclose(file);
  chunk->isDirty = false;
  return chunk;
}

struct GetChunkAsyncRequest {
  int x, z;
  LevelChunk* result;
  std::function<void(LevelChunk*)> callback;
};

struct SaveChunkAsyncRequest {
  LevelChunk* chunk;
  std::function<void()> callback;
};

void ChunkStorage::getChunkAsync(int x, int z,
                                std::function<void(LevelChunk*)> callback) {
  auto* bgService = BackgroundTaskService::getInstance();
  if (!bgService) {
    callback(getChunk(x, z));
    return;
  }

  GetChunkAsyncRequest* req = new GetChunkAsyncRequest();
  req->x = x;
  req->z = z;
  req->result = nullptr;
  req->callback = callback;

  const auto success = bgService->submit(
      [this, req]() { req->result = getChunk(req->x, req->z); },
      [req]() {
        req->callback(req->result);
        delete req;
      });

  if (success == INVALID_BG_TASK) {
    TYRA_WARN("BackgroundTaskService queue full, falling back to sync load");
    LevelChunk* chunk = getChunk(x, z);
    callback(chunk);
    delete req;
  }
}

void ChunkStorage::saveChunk(LevelChunk* chunk) {
  if (!chunk) return;

  const std::string chunksPath = worldDir + "/chunks";
  if (!ensureDirExists(worldDir) || !ensureDirExists(chunksPath)) return;

  char filename[256];
  snprintf(filename, sizeof(filename), "%s/chunks/c.%d.%d.tc", worldDir.c_str(), 
           chunk->x / CHUNK_SIZE, chunk->z / CHUNK_SIZE);
  
  gzFile file = gzopen(filename, "wb");
  if (!file) return;

  TyraCraft::SaveSerializer serializer(file);
  
  // Write header mask
  serializer.WriteUInt8(chunk->loadedSectionsMask);

  // Write sections
  for (int i = 0; i < OVERWORLD_V_DISTANCE_IN_CHUNKS; i++) {
    if (chunk->loadedSectionsMask & (1 << i)) {
      LevelSection* section = chunk->getSection(i);
      serializer.WriteBuffer(section->blocks, CHUNK_LENGTH);
      serializer.WriteBuffer(section->lightData, CHUNK_LENGTH);
      serializer.WriteBuffer(section->metaData, CHUNK_LENGTH);
    }
  }

  chunk->isDirty = false;
}

void ChunkStorage::saveChunkAsync(LevelChunk* chunk,
                                 std::function<void()> callback) {
  if (!chunk) {
    if (callback) callback();
    return;
  }

  auto* bgService = BackgroundTaskService::getInstance();
  if (!bgService) {
    saveChunk(chunk);
    if (callback) callback();
    return;
  }

  SaveChunkAsyncRequest* req = new SaveChunkAsyncRequest();
  req->chunk = chunk;
  req->callback = callback;

  const auto success = bgService->submit(
      [this, req]() { saveChunk(req->chunk); },
      [req]() {
        if (req->callback) req->callback();
        delete req;
      });

  if (success == INVALID_BG_TASK) {
    TYRA_WARN("BackgroundTaskService queue full, falling back to sync save");
    saveChunk(chunk);
    if (callback) callback();
    delete req;
  }
}

void ChunkStorage::tick() {
}

void ChunkStorage::flush() {}

void ChunkStorage::waitForAll() {}

void ChunkStorage::waitIfTooManyQueuedChunks() {}
