#include "managers/save/save_serializer.hpp"

namespace TyraCraft {

SaveSerializer::SaveSerializer(gzFile file) 
    : file(file), buffer(nullptr), bufferSize(0), bufferPos(0), bytesRead(0), bytesWritten(0) {
}

SaveSerializer::SaveSerializer(void* buffer, size_t size)
    : file(nullptr), buffer((uint8_t*)buffer), bufferSize(size), bufferPos(0), bytesRead(0), bytesWritten(0) {
}

SaveSerializer::~SaveSerializer() {
  // Note: We do NOT close the file here - caller is responsible for that
}

SaveResult SaveSerializer::WriteRaw(const void* data, size_t size) {
  if (!IsValid()) {
    return SaveResult::Failure("Invalid file handle");
  }
  
  if (file) {
    int written = gzwrite(file, (void*)data, size);
    if (written != (int)size) {
      return SaveResult::Failure("Failed to write bytes to save file");
    }
  } else if (buffer) {
    if (bufferPos + size > bufferSize) {
      return SaveResult::Failure("Buffer overflow while writing to save buffer");
    }
    memcpy(buffer + bufferPos, data, size);
    bufferPos += size;
  } else {
    return SaveResult::Failure("No file or buffer assigned to serializer");
  }
  
  bytesWritten += size;
  return SaveResult::Success();
}

SaveResult SaveSerializer::ReadRaw(void* data, size_t size) {
  if (!IsValid()) {
    return SaveResult::Failure("Invalid file handle");
  }
  
  if (file) {
    int read = gzread(file, data, size);
    if (read != (int)size) {
      if (read < 0) {
        return SaveResult::Failure("Error reading from save file");
      }
      return SaveResult::Failure("Incomplete read - file may be corrupted or truncated");
    }
  } else if (buffer) {
    if (bufferPos + size > bufferSize) {
      return SaveResult::Failure("Buffer underrun while reading from save buffer");
    }
    memcpy(data, buffer + bufferPos, size);
    bufferPos += size;
  } else {
    return SaveResult::Failure("No file or buffer assigned to serializer");
  }
  
  bytesRead += size;
  return SaveResult::Success();
}

SaveResult SaveSerializer::WriteMagicNumber() {
  return WriteUInt32(MAGIC_NUMBER);
}

SaveResult SaveSerializer::ValidateMagicNumber() {
  uint32_t magic;
  SaveResult result = ReadUInt32(&magic);
  if (!result) {
    return result;
  }
  
  if (magic != MAGIC_NUMBER) {
    return SaveResult::Failure("Invalid save file format - magic number mismatch");
  }
  
  return SaveResult::Success();
}

SaveResult SaveSerializer::WriteInt32(int32_t value) {
  return WriteRaw(&value, sizeof(int32_t));
}

SaveResult SaveSerializer::WriteUInt32(uint32_t value) {
  return WriteRaw(&value, sizeof(uint32_t));
}

SaveResult SaveSerializer::WriteUInt16(uint16_t value) {
  return WriteRaw(&value, sizeof(uint16_t));
}

SaveResult SaveSerializer::WriteUInt8(uint8_t value) {
  return WriteRaw(&value, sizeof(uint8_t));
}

SaveResult SaveSerializer::WriteFloat(float value) {
  return WriteRaw(&value, sizeof(float));
}

SaveResult SaveSerializer::ReadInt32(int32_t* outValue) {
  return ReadRaw(outValue, sizeof(int32_t));
}

SaveResult SaveSerializer::ReadUInt32(uint32_t* outValue) {
  return ReadRaw(outValue, sizeof(uint32_t));
}

SaveResult SaveSerializer::ReadUInt16(uint16_t* outValue) {
  return ReadRaw(outValue, sizeof(uint16_t));
}

SaveResult SaveSerializer::ReadUInt8(uint8_t* outValue) {
  return ReadRaw(outValue, sizeof(uint8_t));
}

SaveResult SaveSerializer::ReadFloat(float* outValue) {
  return ReadRaw(outValue, sizeof(float));
}

SaveResult SaveSerializer::WriteString(const std::string& str, uint16_t maxLength) {
  uint16_t strLen = str.size();
  
  if (strLen > maxLength) {
    return SaveResult::Failure("String too long for save file");
  }
  
  SaveResult result = WriteUInt16(strLen);
  if (!result) return result;
  
  if (strLen > 0) {
    result = WriteRaw(str.data(), strLen);
  }
  
  return result;
}

SaveResult SaveSerializer::ReadString(std::string* outStr, uint16_t maxLength) {
  uint16_t strLen;
  SaveResult result = ReadUInt16(&strLen);
  if (!result) return result;
  
  if (strLen > maxLength) {
    return SaveResult::Failure("String size exceeds maximum allowed length");
  }
  
  if (strLen > 0) {
    char buffer[256];  // Temporary buffer for reading
    result = ReadRaw(buffer, strLen);
    if (!result) return result;
    
    outStr->assign(buffer, strLen);
  } else {
    outStr->clear();
  }
  
  return SaveResult::Success();
}

SaveResult SaveSerializer::WriteVec4(const Tyra::Vec4& vec) {
  SaveResult result = WriteFloat(vec.x);
  if (!result) return result;
  result = WriteFloat(vec.y);
  if (!result) return result;
  result = WriteFloat(vec.z);
  if (!result) return result;
  return WriteFloat(vec.w);
}

SaveResult SaveSerializer::ReadVec4(Tyra::Vec4* outVec) {
  SaveResult result = ReadFloat(&outVec->x);
  if (!result) return result;
  result = ReadFloat(&outVec->y);
  if (!result) return result;
  result = ReadFloat(&outVec->z);
  if (!result) return result;
  return ReadFloat(&outVec->w);
}

SaveResult SaveSerializer::WriteBuffer(const void* data, size_t size) {
  return WriteRaw(data, size);
}

SaveResult SaveSerializer::ReadBuffer(void* data, size_t size) {
  return ReadRaw(data, size);
}

}  // namespace TyraCraft
