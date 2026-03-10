#pragma once
#include <string>

namespace TyraCraft {

/**
 * Result type for save/load operations.
 * Provides structured error reporting instead of silent failures.
 */
struct SaveResult {
  bool success;
  std::string errorMessage;
  
  SaveResult() : success(true), errorMessage("") {}
  
  static SaveResult Success() {
    return SaveResult();
  }
  
  static SaveResult Failure(const std::string& msg) {
    SaveResult result;
    result.success = false;
    result.errorMessage = msg;
    return result;
  }
  
  // Helper to check if operation succeeded
  operator bool() const { return success; }
};

}  // namespace TyraCraft
