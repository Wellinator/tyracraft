#pragma once
#include <zlib.h>
#include <string>
#include <stdint.h>
#include "managers/save/save_result.hpp"
#include "tyra"

namespace TyraCraft {

/**
 * Safe wrapper around gzip file I/O operations.
 * Validates all read/write operations and provides detailed error reporting.
 * 
 * Critical for PS2 constraints:
 * - Detects truncated files (incomplete world data)
 * - Validates string sizes before allocation (prevents OOM)
 * - Provides bounds checking on all reads
 * - Tracks file position for debugging
 */
class SaveSerializer {
 public:
  SaveSerializer(gzFile file);
  ~SaveSerializer();
  
  // Primitive type I/O with validation
  SaveResult WriteInt32(int32_t value);
  SaveResult WriteUInt32(uint32_t value);
  SaveResult WriteUInt16(uint16_t value);
  SaveResult WriteUInt8(uint8_t value);
  SaveResult WriteFloat(float value);
  
  SaveResult ReadInt32(int32_t* outValue);
  SaveResult ReadUInt32(uint32_t* outValue);
  SaveResult ReadUInt16(uint16_t* outValue);
  SaveResult ReadUInt8(uint8_t* outValue);
  SaveResult ReadFloat(float* outValue);
  
  // String I/O with size limits
  static const uint16_t MAX_SAVE_NAME_LENGTH = 256;
  static const uint16_t MAX_TEXTURE_PACK_NAME = 128;
  
  SaveResult WriteString(const std::string& str, uint16_t maxLength);
  SaveResult ReadString(std::string* outStr, uint16_t maxLength);
  
  // Vec4 I/O (player position, camera)
  SaveResult WriteVec4(const Tyra::Vec4& vec);
  SaveResult ReadVec4(Tyra::Vec4* outVec);
  
  // Bulk data I/O (world blocks, light data, metadata)
  SaveResult WriteBuffer(const void* data, size_t size);
  SaveResult ReadBuffer(void* data, size_t size);
  
  // File position tracking (for debugging)
  uint64_t GetBytesRead() const { return bytesRead; }
  uint64_t GetBytesWritten() const { return bytesWritten; }
  
  // Magic number validation
  static const uint32_t MAGIC_NUMBER = 0x54435753;  // "TCWS" - TyraCraft World Save
  SaveResult WriteMagicNumber();
  SaveResult ValidateMagicNumber();
  
  bool IsValid() const { return file != nullptr; }
  
 private:
  gzFile file;
  uint64_t bytesRead;
  uint64_t bytesWritten;
  
  // Internal validated read/write
  SaveResult WriteRaw(const void* data, size_t size);
  SaveResult ReadRaw(void* data, size_t size);
};

}  // namespace TyraCraft
