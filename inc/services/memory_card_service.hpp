#pragma once

#include <tamtypes.h>
#include <libmc.h>
#include <string>
#include <vector>
#include "singleton.hpp"

namespace TyraCraft {

class MemoryCardService : public Singleton<MemoryCardService> {
 public:
  static const char* MC_ROOT_DIR;

  MemoryCardService();
  ~MemoryCardService();

  /** 
   * Initialize Memory Card modules and library. 
   * Returns true if modules were loaded successfully.
   */
  bool init();

  /** Check if a memory card is inserted in the specified slot/port */
  bool isAvailable(int port = 0, int slot = 0);

  /** Get free space in kilobytes */
  int getFreeSpace(int port = 0, int slot = 0);

  bool ensureDirectoryExists(int port = 0, int slot = 0, const char* saveName = nullptr);

  /**
   * @brief Copy icon.sys and icon.icn from res/ to the specified MC directory
   */
  bool installIcon(const std::string& targetDir, int port = 0, int slot = 0);

  /** Get the list of TyraCraft saves on the memory card */
  std::vector<std::string> listSaves(int port = 0, int slot = 0);

  /** Get the base path for MC saves (e.g., "mc0:/TyraCraft") */
  std::string getMcPath(int port = 0, int slot = 0);

  /** Load a custom IRX from a file */
  static int loadIrx(const char* filename);

 private:
  bool initialized;
  
  void loadModules();
};

}  // namespace TyraCraft
