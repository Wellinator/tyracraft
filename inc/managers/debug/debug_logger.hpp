#pragma once

#include <string>
#include <vector>
#include <cstdarg>
#include "singleton.hpp"

namespace TyraCraft {

/**
 * @brief Singleton class to capture and store debug log messages for on-screen display.
 */
class DebugLogger : public Singleton<DebugLogger> {
 public:
  DebugLogger();
  ~DebugLogger();

  /**
   * @brief Adds a new log message to the circular buffer.
   * @param message The message to add.
   */
  void addLog(const std::string& message);
  void addLog(const char* format, ...);

  /**
   * @brief Returns a reference to the log buffer.
   */
  const std::vector<std::string>& getLogs() const { return logs; }

  /**
   * @brief Clears all captured logs.
   */
  void clear();

 private:
  static const size_t MAX_LOG_LINES = 100;
  std::vector<std::string> logs;
};

}  // namespace TyraCraft
