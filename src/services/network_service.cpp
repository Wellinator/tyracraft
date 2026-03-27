#include "services/network_service.hpp"
#include <sifrpc.h>
#include <loadfile.h>
#include "services/exception_handler.hpp"


extern "C" {
#include <netman.h>
#include <ps2ip.h>
}

#include <stdio.h>
#include <string.h>
#include "debug.hpp"
#include <file/file_utils.hpp>

namespace TyraCraft {

NetworkService::NetworkService()
    : initialized(false), connected(false), ipAddr("0.0.0.0") {}

NetworkService::~NetworkService() {
  if (initialized) {
    NetManDeinit();
  }
}

bool NetworkService::init() {
  if (initialized) return true;

  TYRA_LOG("--------------------------------------------------");
  TYRA_LOG("| Initializing Network Service...                |");

  // 1. Load IRX modules
  if (!loadModules()) {
    TYRA_ERROR("| Failed to load some network modules!          |");
    TYRA_LOG("--------------------------------------------------");
    return false;
  }

  // 2. Initialize network stack
  if (!setupNetwork()) {
    TYRA_ERROR("| Failed to initialize network stack!           |");
    TYRA_LOG("--------------------------------------------------");
    return false;
  }

  initialized = true;
  TYRA_LOG("| Network Service initialized successfully.      |");
  TYRA_LOG("--------------------------------------------------");
  return true;
}

bool NetworkService::loadModules() {
  const char* modules[] = {"ps2dev9.irx", "netman.irx", "smap.irx", "ps2ip-nm.irx"};
  bool allLoaded = true;

  TYRA_LOG("| Loading Network IRX modules...                 |");

  std::string irxDir = Tyra::FileUtils::fromCwd("irx/");

  for (int i = 0; i < 4; i++) {
    const char* mod = modules[i];
    std::string path = irxDir + mod;

    int id = loadIrx(path.c_str());
    if (id >= 0) {
      TYRA_LOG("|   - ", mod, " : SUCCESS                         |");
    } else {
      TYRA_LOG("|   - ", mod, " : FAILED (Local)                |");
      
      // Critical modules fallback
      bool fallback = false;
      if (strcmp(mod, "ps2dev9.irx") == 0) {
        if (loadIrx("rom0:PS2DEV9") >= 0) fallback = true;
      } else if (strcmp(mod, "smap.irx") == 0) {
        if (loadIrx("rom0:SMAP") >= 0) fallback = true;
      }

      if (!fallback) {
        TYRA_ERROR("|     -> Error loading CRITICAL module!        |");
        allLoaded = false;
      } else {
        TYRA_LOG("|     -> Fallback  : SUCCESS (ROM)             |");
      }
    }
  }

  // Load udptty for resilient logging (optional)
  if (loadIrx((irxDir + "udptty.irx").c_str()) >= 0) {
    TYRA_LOG("|   - udptty.irx : SUCCESS (UDP Logger ready)  |");
  }

  return allLoaded;
}

int NetworkService::loadIrx(const char* filename) {
  int id = SifLoadModule(filename, 0, NULL);
  if (id < 0) {
    // Some modules return negative values upon successful initialization but the RPC call itself was fine.
    // However, for SifLoadModule, anything negative is generally an error finding or starting the module.
    // Error codes: -200 (File not found), -203 (Initialization failed), etc.
    return id; 
  }
  return id;
}

bool NetworkService::setupNetwork() {
  TYRA_LOG("| Configuring network stack (DHCP)...           |");

  if (NetManInit() < 0) {
    TYRA_ERROR("| NetManInit failed!                            |");
    return false;
  }

  // Initialize with 0.0.0.0 to trigger DHCP
  struct ip4_addr ip, nm, gw;
  ip.addr = 0;
  nm.addr = 0;
  gw.addr = 0;

  if (ps2ipInit(&ip, &nm, &gw) < 0) {
    TYRA_ERROR("| ps2ipInit failed!                             |");
    return false;
  }

  // In PS2SDK, once smap is loaded and ps2ip initialized, 
  // it starts looking for a link and DHCP lease.
  return true;
}

void NetworkService::update() {
  if (!initialized) return;

  // Check link status
  int linkStatus = NetManGetGlobalNetIFLinkState();
  if (linkStatus == NETMAN_NETIF_ETH_LINK_STATE_UP) {
    if (!connected) {
      TYRA_LOG("| Network Link: UP                              |");
    }
    connected = true;

    // Refresh IP address if it was pending
    t_ip_info ipInfo;
    if (ps2ip_getconfig("sm0", &ipInfo) == 0) {
       char ipBuffer[16];
       u32 ip = ipInfo.ipaddr.s_addr;
       sprintf(ipBuffer, "%d.%d.%d.%d", 
               (u8)(ip & 0xFF), 
               (u8)((ip >> 8) & 0xFF), 
               (u8)((ip >> 16) & 0xFF), 
               (u8)((ip >> 24) & 0xFF));
       ipAddr = std::string(ipBuffer);
    }
  } else {
    if (connected) {
       TYRA_LOG("| Network Link: DOWN                            |");
    }
    connected = false;
    ipAddr = "Disconnected";
  }
}

void NetworkService::installExceptionHandler() {
  ExceptionHandler::install();
}


}  // namespace TyraCraft
