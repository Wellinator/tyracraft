#include "utils.hpp"
#include <file/file_utils.hpp>
#include "services/memory_card_service.hpp"
#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
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

bool MemoryCardService::isAvailable(int port, int slot) {
  if (!initialized) return false;

  int type, free, format, ret;
  // libmc: mcGetInfo(port, slot, ...)
  mcGetInfo(port, slot, &type, &free, &format);
  mcSync(MC_WAIT, NULL, &ret);

  // If type is not 0, there's a card
  return type != 0;
}

int MemoryCardService::getFreeSpace(int port, int slot) {
  if (!initialized) return -1;

  int type, free, format, ret;
  // libmc: mcGetInfo(port, slot, ...)
  mcGetInfo(port, slot, &type, &free, &format);
  mcSync(MC_WAIT, NULL, &ret);

  return free; // in clusters (each cluster = 512 bytes on a standard PS2 MC)
}

bool MemoryCardService::ensureDirectoryExists(int port, int slot) {
  if (!initialized) return false;

  std::string relRootDir = MC_ROOT_DIR;
  std::string relSavesDir = relRootDir + "/" + MC_SAVES_DIR;

  // We skip directoryExists check for MC because opendir() can crash on mcX:
  // Instead, we just try to create the directories and check the results.
  
  int ret;
  TYRA_LOG("Ensuring MC directory structure: /", relRootDir.c_str());
  
  // libmc: mcMkDir(port, slot, name)
  // Try to create root dir
  mcMkDir(port, slot, relRootDir.c_str());
  mcSync(MC_WAIT, NULL, &ret);
  // ret >= 0 means created, -17 (EEXIST) means already exists, both are fine.

  // Try to create saves dir
  mcMkDir(port, slot, relSavesDir.c_str());
  mcSync(MC_WAIT, NULL, &ret);

  // Install icon files
  installIcon(port, slot);

  return true;
}

bool MemoryCardService::installIcon(int port, int slot) {
  const char* iconFiles[] = {"icon.sys", "icon.icn"};
  std::string mcPath = getMcPath(port, slot);

  // Enter the TyraCraft directory on MC
  // libmc: mcChdir(port, slot, newDir, currentDir)
  // NOTE: currentDir MUST be a valid buffer — libmc always writes to it,
  //       even when the caller doesn't need the value. Passing NULL causes TLB Miss.
  int ret;
  char prevDir[256] = {0};
  mcChdir(port, slot, MC_ROOT_DIR, prevDir);
  mcSync(MC_WAIT, NULL, &ret);

  for (const char* iconFile : iconFiles) {
    std::string destPath = mcPath + "/" + iconFile;
    
    // Try both /res/ and root directory
    std::string srcPath = Tyra::FileUtils::fromCwd("res/") + iconFile;
    if (!Utils::fileExists(srcPath)) {
      srcPath = Tyra::FileUtils::fromCwd(iconFile);
    }
    
    // Final check
    if (!Utils::fileExists(srcPath)) {
        srcPath = Tyra::FileUtils::fromCwd(std::string("/") + iconFile);
    }

    if (Utils::fileExists(srcPath)) {
      TYRA_LOG("Installing ", iconFile, " to MC from ", srcPath);
        
      FILE* src = fopen(srcPath.c_str(), "rb");
      if (src) {
        // libmc: mcOpen(port, slot, name, mode) — result (fd) returned via mcSync
        int fd, writeRet;
        mcOpen(port, slot, iconFile, O_WRONLY | O_CREAT);
        mcSync(MC_WAIT, NULL, &fd);

        if (fd >= 0) {
          char buffer[1024];
          size_t bytes;
          while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            // libmc: mcWrite(fd, buffer, size)
            mcWrite(fd, buffer, (int)bytes);
            mcSync(MC_WAIT, NULL, &writeRet);
          }
          // Flush before close to ensure data is committed
          mcFlush(fd);
          mcSync(MC_WAIT, NULL, &ret);
          // libmc: mcClose(fd)
          mcClose(fd);
          mcSync(MC_WAIT, NULL, &ret);
        }
        fclose(src);
      }
    } else {
      TYRA_LOG("Warning: ", srcPath, " not found.");
    }
  }

  // Return to root
  char prevDir2[256] = {0};
  mcChdir(port, slot, "/", prevDir2);
  mcSync(MC_WAIT, NULL, &ret);

  return true;
}

std::string MemoryCardService::getMcPath(int port, int slot) {
  char path[32];
  sprintf(path, "mc%d:/%s", port, MC_ROOT_DIR);
  return std::string(path);
}

}  // namespace TyraCraft
