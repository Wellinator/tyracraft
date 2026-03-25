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

bool MemoryCardService::ensureDirectoryExists(int port, int slot, const char* saveName) {
  if (!initialized) return false;

  int ret;
  std::string relRootDir = MC_ROOT_DIR;

  if (saveName) {
    // Create save directory: TyraCraft_<saveName>
    std::string saveDir = relRootDir + "_" + saveName;
    TYRA_LOG("Ensuring MC save directory: /", saveDir.c_str());
    
    mcMkDir(port, slot, saveDir.c_str());
    mcSync(MC_WAIT, NULL, &ret);

    // Install icons into the specific save directory
    installIcon(getMcPath(port, slot) + "_" + saveName, port, slot);
  } else {
    // Default root directory
    TYRA_LOG("Ensuring MC root directory: /", relRootDir.c_str());
    mcMkDir(port, slot, relRootDir.c_str());
    mcSync(MC_WAIT, NULL, &ret);

    // Install icons into the root directory
    installIcon(getMcPath(port, slot), port, slot);
  }

  return true;
}

bool MemoryCardService::installIcon(const std::string& targetDir, int port, int slot) {
  const char* iconFiles[] = {"icon.sys", "icon.icn"};

  // Enter the target directory on MC
  // Extract relative path from targetDir (e.g. "mc0:/TyraCraft_World" -> "TyraCraft_World")
  size_t colonPos = targetDir.find(':');
  std::string relDir = (colonPos != std::string::npos) ? targetDir.substr(colonPos + 2) : targetDir;

  int ret;
  char prevDir[256] = {0};
  mcChdir(port, slot, relDir.c_str(), prevDir);
  mcSync(MC_WAIT, NULL, &ret);

  for (const char* iconFile : iconFiles) {
    std::string destPath = targetDir + "/" + iconFile;
    
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

std::vector<std::string> MemoryCardService::listSaves(int port, int slot) {
  std::vector<std::string> saves;
  if (!initialized) return saves;

  sceMcTblGetDir table[10] __attribute__((aligned(64)));
  int count;

  // Search for directories starting with TyraCraft_
  std::string searchPattern = std::string(MC_ROOT_DIR) + "_*";
  mcGetDir(port, slot, searchPattern.c_str(), 0, 10, table);
  mcSync(MC_WAIT, NULL, &count);

  if (count > 0) {
    for (int i = 0; i < count; i++) {
      // Ensure it's a directory (bit 5 in entry mode)
      if (table[i].AttrFile & MC_ATTR_SUBDIR) {
        saves.push_back(std::string(reinterpret_cast<const char*>(table[i].EntryName)));
      }
    }
  }

  return saves;
}

}  // namespace TyraCraft
