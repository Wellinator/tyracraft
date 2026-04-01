#include "entities/chunk_storage.hpp"
#include <zlib.h>
#include "managers/save/save_serializer.hpp"
#include <cstdio>
#include "debug.hpp"
#include "managers/background_task_service.hpp"
#include "utils.hpp"

uint8_t* ChunkStorage::ioTransferBuffer = nullptr;

static bool ensureDirExists(const std::string& path) {
  return Utils::makeDirectoryRecursive(path);
}

ChunkStorage::ChunkStorage(const std::string& worldDir) : worldDir(Utils::normalizePath(worldDir)) {
  ensureDirExists(worldDir);
  std::string chunksPath = worldDir + "/chunks";
  ensureDirExists(chunksPath);
  allocateIOBuffer();
}

void ChunkStorage::allocateIOBuffer() {
  if (ioTransferBuffer) return;
  // 128KB is enough for uncompressed (72KB) and compressed voxel data
  ioTransferBuffer = new uint8_t[128 * 1024];
}

ChunkStorage::~ChunkStorage() {}

LevelChunk* ChunkStorage::getChunk(int x, int z) {
  // Try to load from region binary format
  return loadChunkFromRegion(x, z);
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
    TYRA_WARN("BackgroundTaskService queue full, skipping async load for %d, %d", x, z);
    callback(nullptr);
    delete req;
  }
}

void ChunkStorage::saveChunk(LevelChunk* chunk) {
  if (!chunk) return;
  saveChunkToRegion(chunk);
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

std::string ChunkStorage::getRegionFilePath(int x, int z) {
  // x, z are chunk indices (0 to 15 for 256 world)
  int rx = x / CHUNKS_PER_REGION_AXIS;
  int rz = z / CHUNKS_PER_REGION_AXIS;
  
  char filename[128];
  snprintf(filename, sizeof(filename), "/chunks/reg.%d.%d.bin", rx, rz);
  
  return worldDir + filename;
}

int ChunkStorage::getChunkIndexInRegion(int x, int z) {
  // x, z are chunk indices (0 to 15 for 256 world)
  int lx = x % CHUNKS_PER_REGION_AXIS;
  int lz = z % CHUNKS_PER_REGION_AXIS;
  
  return (lz * CHUNKS_PER_REGION_AXIS) + lx;
}

LevelChunk* ChunkStorage::loadChunkFromRegion(int x, int z) {
  std::string path = getRegionFilePath(x, z);
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) return nullptr;

  RegionHeader header;
  if (fread(&header, 1, REGION_HEADER_SIZE, file) != REGION_HEADER_SIZE) {
    fclose(file);
    return nullptr;
  }

  if (header.magic != 0x52434654) { // 'TCFR'
    fclose(file);
    return nullptr;
  }

  int index = getChunkIndexInRegion(x, z);
  uint32_t compSize = header.sizes[index];
  if (compSize == 0) {
    fclose(file);
    return nullptr;
  }

  // Seek to fixed slot
  fseek(file, REGION_HEADER_SIZE + (index * SLOT_SIZE), SEEK_SET);
  
  uint8_t* compressedBuffer = new uint8_t[compSize];
  fread(compressedBuffer, 1, compSize, file);
  fclose(file);

  unsigned long destLen = 128 * 1024; // Use our 128KB buffer size
  int res = uncompress(ioTransferBuffer, &destLen, compressedBuffer, compSize);
  delete[] compressedBuffer;

  if (res != Z_OK) {
    TYRA_ERROR("Zlib failure during Region load (Error: %d)", res);
    return nullptr;
  }

  TyraCraft::SaveSerializer serializer(ioTransferBuffer, destLen);
  LevelChunk* chunk = new LevelChunk(x * CHUNK_SIZE, z * CHUNK_SIZE);

  uint8_t mask;
  serializer.ReadUInt8(&mask);
  for (int i = 0; i < OVERWORLD_V_DISTANCE_IN_CHUNKS; i++) {
    if (mask & (1 << i)) {
      chunk->allocateSection(i);
      LevelSection* section = chunk->getSection(i);
      serializer.ReadBuffer(section->blocks, CHUNK_LENGTH);
      serializer.ReadBuffer(section->lightData, CHUNK_LENGTH);
      serializer.ReadBuffer(section->metaData, CHUNK_LENGTH);
    }
  }

  chunk->isDirty = false;
  return chunk;
}

void ChunkStorage::saveChunkToRegion(LevelChunk* chunk) {
  int x = chunk->x / CHUNK_SIZE;
  int z = chunk->z / CHUNK_SIZE;
  
  // 1. Serialize to buffer
  TyraCraft::SaveSerializer serializer(ioTransferBuffer, 128 * 1024);
  serializer.WriteUInt8(chunk->loadedSectionsMask);
  for (int i = 0; i < OVERWORLD_V_DISTANCE_IN_CHUNKS; i++) {
    if (chunk->loadedSectionsMask & (1 << i)) {
      LevelSection* section = chunk->getSection(i);
      serializer.WriteBuffer(section->blocks, CHUNK_LENGTH);
      serializer.WriteBuffer(section->lightData, CHUNK_LENGTH);
      serializer.WriteBuffer(section->metaData, CHUNK_LENGTH);
    }
  }
  
  uint32_t uncompressedSize = serializer.GetBytesWritten();

  // 2. Fast Compression (Level 1)
  // Re-use ioTransferBuffer? No, we need source and dest.
  // We'll allocate a temporary buffer for compressed data (will be 32KB anyway)
  uint8_t compressedBuffer[SLOT_SIZE]; 
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  
  if (deflateInit2(&strm, Z_BEST_SPEED, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
      TYRA_ERROR("DeflateInit failed");
      return;
  }
  
  strm.next_in = ioTransferBuffer;
  strm.avail_in = uncompressedSize;
  strm.next_out = compressedBuffer;
  strm.avail_out = SLOT_SIZE;
  
  int res = deflate(&strm, Z_FINISH);
  uint32_t compressedSize = SLOT_SIZE - strm.avail_out;
  deflateEnd(&strm);

  if (res != Z_STREAM_END) {
    TYRA_ERROR("Chunk compression exceeded 32KB slot at %d, %d", x, z);
    return;
  }

  // 3. Update Region File
  std::string path = getRegionFilePath(x, z);
  FILE* file = fopen(path.c_str(), "rb+");
  if (!file) {
    // Create new region file
    file = fopen(path.c_str(), "wb+");
    if (!file) return;

    RegionHeader header;
    header.magic = 0x52434654;
    header.version = 2;
    for (int i = 0; i < CHUNKS_PER_REGION_AXIS * CHUNKS_PER_REGION_AXIS; i++) header.sizes[i] = 0;
    
    fwrite(&header, 1, REGION_HEADER_SIZE, file);
    
    // Pre-allocate slots with zeros
    uint8_t zero[1024]; 
    memset(zero, 0, 1024);
    for (int i = 0; i < (SLOT_SIZE * CHUNKS_PER_REGION_AXIS * CHUNKS_PER_REGION_AXIS) / 1024; i++) {
        fwrite(zero, 1, 1024, file);
    }
    fseek(file, 0, SEEK_SET);
  }

  // Load Header
  RegionHeader header;
  fseek(file, 0, SEEK_SET);
  fread(&header, 1, REGION_HEADER_SIZE, file);

  int index = getChunkIndexInRegion(x, z);
  header.sizes[index] = compressedSize;

  // Save Header
  fseek(file, 0, SEEK_SET);
  fwrite(&header, 1, REGION_HEADER_SIZE, file);

  // Save Data in Slot
  fseek(file, REGION_HEADER_SIZE + (index * SLOT_SIZE), SEEK_SET);
  fwrite(compressedBuffer, 1, compressedSize, file);

  fclose(file);
  chunk->isDirty = false;
}

void ChunkStorage::waitForAll() {}

void ChunkStorage::waitIfTooManyQueuedChunks() {}
