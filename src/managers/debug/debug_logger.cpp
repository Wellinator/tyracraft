#include "managers/debug/debug_logger.hpp"
#include <algorithm>

namespace TyraCraft {

DebugLogger::DebugLogger() {
  logs.reserve(MAX_LOG_LINES);
}

DebugLogger::~DebugLogger() {
  logs.clear();
}

void DebugLogger::addLog(const std::string& message) {
  if (logs.size() >= MAX_LOG_LINES) {
    logs.erase(logs.begin());
  }
  logs.push_back(message);
}

void DebugLogger::addLog(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  addLog(std::string(buffer));
}

void DebugLogger::clear() {
  logs.clear();
}

}  // namespace TyraCraft
