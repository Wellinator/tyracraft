#include "utils.hpp"
#include <file/file_utils.hpp>
#include "services/memory_card_service.hpp"
#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <string.h>
#include "utils.hpp"
#include "debug.hpp"

namespace TyraCraft {

const char* MemoryCardService::MC_ROOT_DIR = "TyraCraft";
const char* MemoryCardService::MC_SAVES_DIR = "saves";

MemoryCardService::MemoryCardService() : initialized(false) {}

MemoryCardService::~MemoryCardService() {}

bool MemoryCardService::init() {
  if (initialized) return true;

  TYRA_LOG("--------------------------------------------------");
  TYRA_LOG("| Initializing Memory Card Service...            |");
  
  // Load IRX modules first
  loadModules();

  // Initialize MC library
  int ret = mcInit(MC_TYPE_XMC);
  if (ret < 0) {
    TYRA_ERROR("| Failed to initialize MC library!              |");
    TYRA_LOG("--------------------------------------------------");
    return false;
  }

  initialized = true;
  TYRA_LOG("| MC library initialized successfully.           |");
  TYRA_LOG("--------------------------------------------------");
  return true;
}

void MemoryCardService::loadModules() {
  const char* modules[] = {"mcman.irx", "mcserv.irx"};
  
  TYRA_LOG("| Loading IRX modules...                         |");

  for (int i = 0; i < 2; i++) {
    const char* mod = modules[i];
    std::string path = Tyra::FileUtils::fromCwd("irx/") + mod;
    
    int id = loadIrx(path.c_str());
    if (id >= 0) {
      TYRA_LOG("|   - ", mod, " : SUCCESS (Local)           |");
    } else {
      TYRA_LOG("|   - ", mod, " : FAILED (Local)            |");
      // Fallback to standard ROM modules
      int fallbackId = -1;
      if (strcmp(mod, "mcman.irx") == 0) fallbackId = loadIrx("rom0:XMCMAN");
      else if (strcmp(mod, "mcserv.irx") == 0) fallbackId = loadIrx("rom0:XMCSERV");

      if (fallbackId >= 0) {
        TYRA_LOG("|     -> Fallback  : SUCCESS (ROM)             |");
      } else {
        TYRA_ERROR("|     -> Fallback  : FAILED (ROM)              |");
      }
    }
  }
}

int MemoryCardService::loadIrx(const char* filename) {
  int id = SifLoadModule(filename, 0, NULL);
  if (id < 0) {
    return -1;
  }
  return id;
}

bool MemoryCardService::isAvailable(int slot, int port) {
  if (!initialized) return false;

  int type, free, format, ret;
  mcGetInfo(slot, port, &type, &free, &format);
  mcSync(MC_WAIT, NULL, &ret);

  // If type is not 0, there's a card
  return type != 0;
}

int MemoryCardService::getFreeSpace(int slot, int port) {
  if (!initialized) return -1;

  int type, free, format, ret;
  mcGetInfo(slot, port, &type, &free, &format);
  mcSync(MC_WAIT, NULL, &ret);

  return free; // in KB
}

bool MemoryCardService::ensureDirectoryExists(int slot, int port) {
  if (!initialized) return false;

  std::string rootPath = getMcPath(slot, port);
  
  // Check if root dir exists
  if (!Utils::directoryExists(rootPath)) {
    TCLOG("Creating MC root directory: %s", rootPath.c_str());
    if (mcMkDir(slot, port, MC_ROOT_DIR) < 0) {
        mcSync(MC_WAIT, NULL, NULL); // wait anyway
    } else {
        mcSync(MC_WAIT, NULL, NULL);
    }
  }

  std::string savesPath = rootPath + "/" + MC_SAVES_DIR;
  if (!Utils::directoryExists(savesPath)) {
    TCLOG("Creating MC saves directory: %s", savesPath.c_str());
    // mcMkDir requires the base directory to exist, which we just handled
    std::string relSavesDir = std::string(MC_ROOT_DIR) + "/" + MC_SAVES_DIR;
    if (mcMkDir(slot, port, relSavesDir.c_str()) < 0) {
        mcSync(MC_WAIT, NULL, NULL);
    } else {
        mcSync(MC_WAIT, NULL, NULL);
    }
  }

  return true;
}

std::string MemoryCardService::getMcPath(int slot, int port) {
  char path[32];
  sprintf(path, "mc%d:/%s", slot, MC_ROOT_DIR);
  return std::string(path);
}

}  // namespace TyraCraft
