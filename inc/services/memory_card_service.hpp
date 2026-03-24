#pragma once

#include <tamtypes.h>
#include <libmc.h>
#include <string>
#include "singleton.hpp"

namespace TyraCraft {

class MemoryCardService : public Singleton<MemoryCardService> {
 public:
  MemoryCardService();
  ~MemoryCardService();

  /** 
   * Initialize Memory Card modules and library. 
   * Returns true if modules were loaded successfully.
   */
  bool init();

  /** Check if a memory card is inserted in the specified slot/port */
  bool isAvailable(int slot = 0, int port = 0);

  /** Get free space in kilobytes */
  int getFreeSpace(int slot = 0, int port = 0);

  /** Create TyraCraft directory on the card if it doesn't exist */
  bool ensureDirectoryExists(int slot = 0, int port = 0);

  /** Get the base path for MC saves (e.g., "mc0:/TyraCraft/") */
  std::string getMcPath(int slot = 0, int port = 0);

  /** Load a custom IRX from a file */
  static int loadIrx(const char* filename);

 private:
  bool initialized;
  
  static const char* MC_ROOT_DIR;
  static const char* MC_SAVES_DIR;

  void loadModules();
};

}  // namespace TyraCraft
