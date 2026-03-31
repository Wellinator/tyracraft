#include "services/network_service.hpp"
#include <sifrpc.h>
#include <loadfile.h>
#include "services/exception_handler.hpp"
#include "managers/settings_manager.hpp"

#include <stdio.h>
#include <string.h>
#include "debug.hpp"
#include <file/file_utils.hpp>

namespace TyraCraft {

// SMAP always registers its lwIP netif as "sm0":
//   netif->name[0]='s', netif->name[1]='m', netif->num=0
const char* const NetworkService::NETIF_NAME = "sm0";

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

NetworkService::NetworkService()
    : initialized(false), connected(false), ipAddr("0.0.0.0"),
      lastTestResult("Not tested"), initRetryCount(0) {}

NetworkService::~NetworkService() {
  if (initialized) {
    ps2ipDeinit();
    NetManDeinit();
  }
}

// ---------------------------------------------------------------------------
// Public: init
// ---------------------------------------------------------------------------

bool NetworkService::init() {
  if (initialized) return true;

  TYRA_LOG("--------------------------------------------------");
  TYRA_LOG("| Initializing Network Service...                |");

  if (!loadModules()) {
    TYRA_ERROR("| Failed to load some network modules!          |");
    TYRA_LOG("--------------------------------------------------");
    return false;
  }

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

// ---------------------------------------------------------------------------
// IRX loading
// ---------------------------------------------------------------------------

bool NetworkService::loadModules() {
  TYRA_LOG("| Loading Network IRX modules...                 |");
  TYRA_LOG("|   Network mode: ", g_settings.eth_dhcp ? "DHCP" : "Static",
           " IP: ", g_settings.eth_ip.c_str());

  std::string irxDir = Tyra::FileUtils::fromCwd("irx/");

  bool ps2dev9 = loadPs2Dev9(irxDir);
  bool netman  = loadNetman(irxDir);
  bool smap    = loadSmap(irxDir);

  loadUdptty(irxDir);

  return ps2dev9 && netman && smap;
}

bool NetworkService::loadPs2Dev9(const std::string& irxDir) {
  std::string path = irxDir + "ps2dev9.irx";
  if (loadIrx(path.c_str()) >= 0) {
    TYRA_LOG("|   - ps2dev9.irx : SUCCESS                    |");
    return true;
  }
  TYRA_ERROR("|   - ps2dev9.irx : FAILED                     |");
  return false;
}

bool NetworkService::loadNetman(const std::string& irxDir) {
  std::string path = irxDir + "netman.irx";
  if (loadIrx(path.c_str()) >= 0) {
    TYRA_LOG("|   - netman.irx : SUCCESS                     |");
    return true;
  }
  TYRA_ERROR("|   - netman.irx : FAILED                      |");
  return false;
}

bool NetworkService::loadSmap(const std::string& irxDir) {
  std::string path = irxDir + "smap.irx";
  if (loadIrx(path.c_str()) >= 0) {
    TYRA_LOG("|   - smap.irx : SUCCESS                       |");
    return true;
  }
  TYRA_ERROR("|   - smap.irx : FAILED                        |");
  return false;
}

bool NetworkService::loadUdptty(const std::string& irxDir) {
  std::string path = irxDir + "udptty.irx";
  if (loadIrx(path.c_str()) >= 0) {
    TYRA_LOG("|   - udptty.irx : SUCCESS                     |");
    return true;
  }
  return false;
}

int NetworkService::loadIrx(const char* filename, int argc, char** argv) {
  if (argc > 0 && argv != NULL) {
    char argBuf[256];
    int pos = 0;
    for (int i = 0; i < argc; i++) {
      int len = strlen(argv[i]);
      if (pos + len + 1 > (int)sizeof(argBuf)) break;
      strcpy(argBuf + pos, argv[i]);
      pos += len + 1;
    }
    return SifLoadModule(filename, pos, argBuf);
  }
  return SifLoadModule(filename, 0, NULL);
}

// ---------------------------------------------------------------------------
// ethApplyNetIFConfig
// ---------------------------------------------------------------------------

int NetworkService::ethApplyNetIFConfig(int mode) {
  int result;
  static int CurrentMode = NETMAN_NETIF_ETH_LINK_MODE_AUTO;

  if (CurrentMode != mode) {
    if ((result = NetManSetLinkMode(mode)) == 0)
      CurrentMode = mode;
  } else {
    result = 0;
  }

  return result;
}

// ---------------------------------------------------------------------------
// ethApplyIPConfig
// ---------------------------------------------------------------------------

int NetworkService::ethApplyIPConfig(int use_dhcp,
                                     const ::ip4_addr* ip,
                                     const ::ip4_addr* netmask,
                                     const ::ip4_addr* gateway) {
  t_ip_info ip_info;
  int result;

  // ps2ip_getconfig returns 1 on success, 0 when interface not found.
  if ((result = ps2ip_getconfig((char*)NETIF_NAME, &ip_info)) >= 1) {
    bool same_ip = true;
    if (!use_dhcp) {
      // Direct 32-bit comparison to avoid type punning / alignment warnings
      if (ip->addr != ip_info.ipaddr.s_addr) same_ip = false;
      if (netmask->addr != ip_info.netmask.s_addr) same_ip = false;
      if (gateway->addr != ip_info.gw.s_addr) same_ip = false;
    }

    if ((use_dhcp != (int)ip_info.dhcp_enabled) || (!use_dhcp && !same_ip)) {
      if (use_dhcp) {
        ip_info.dhcp_enabled = 1;
      } else {
        // Direct assignment to comply with struct definitions properly
        ip_info.ipaddr.s_addr = ip->addr;
        ip_info.netmask.s_addr = netmask->addr;
        ip_info.gw.s_addr = gateway->addr;
        ip_info.dhcp_enabled = 0;
      }
      result = ps2ip_setconfig(&ip_info);
    } else {
      result = 1;
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// setupNetwork
// ---------------------------------------------------------------------------

bool NetworkService::setupNetwork() {
  TYRA_LOG("| Configuring network stack...                  |");

  ::ip4_addr IP, NM, GW;

  if (g_settings.eth_dhcp) {
    TYRA_LOG("|   - Mode: DHCP                                |");
    IP.addr = NM.addr = GW.addr = 0;
  } else {
    TYRA_LOG("|   - Mode: Static IP                           |");
    TYRA_LOG("|   - IP: ", g_settings.eth_ip.c_str());

    int a, b, c, d;
    IP.addr = NM.addr = GW.addr = 0;

    if (sscanf(g_settings.eth_ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
      IP4_ADDR(&IP, a, b, c, d);
    if (sscanf(g_settings.eth_netmask.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
      IP4_ADDR(&NM, a, b, c, d);
    if (sscanf(g_settings.eth_gateway.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
      IP4_ADDR(&GW, a, b, c, d);
  }

  if (ethApplyNetIFConfig(NETMAN_NETIF_ETH_LINK_MODE_AUTO) != 0) {
    TYRA_ERROR("| ethApplyNetIFConfig failed!                   |");
  }

  if (ps2ipInit(&IP, &NM, &GW) < 0) {
    TYRA_ERROR("| ps2ipInit failed!                             |");
    return false;
  }

  if (ethApplyIPConfig(g_settings.eth_dhcp ? 1 : 0, &IP, &NM, &GW) < 1) {
    TYRA_ERROR("| ethApplyIPConfig failed! Interface not ready. |");
  } else {
    if (g_settings.eth_dhcp) {
      TYRA_LOG("| DHCP client started.                          |");
    } else {
      TYRA_LOG("| Static IP configured.                         |");
    }
  }

  TYRA_LOG("| Network stack initialized OK.                 |");
  return true;
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void NetworkService::update() {
  if (!initialized) return;

  int linkStatus =
      NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0);
  bool hasLink = (linkStatus == NETMAN_NETIF_ETH_LINK_STATE_UP);

  static bool lastLinkState = false;
  if (hasLink && !lastLinkState) {
    lastLinkState  = true;
    initRetryCount = 0;
    TYRA_LOG("| Network Link: UP  IF='", NETIF_NAME, "'              |");
  } else if (!hasLink && lastLinkState) {
    TYRA_LOG("| Network Link: DOWN                            |");
    lastLinkState = false;
    connected     = false;
  }

  t_ip_info ipInfo;
  int cfgResult = ps2ip_getconfig((char*)NETIF_NAME, &ipInfo);

  if (cfgResult >= 1) {
    u32 ip = ipInfo.ipaddr.s_addr;
    if (ip != 0) {
      char ipBuffer[16];
      sprintf(ipBuffer, "%d.%d.%d.%d",
              (u8)(ip & 0xFF), (u8)((ip >> 8) & 0xFF),
              (u8)((ip >> 16) & 0xFF), (u8)((ip >> 24) & 0xFF));
      std::string newIp(ipBuffer);
      if (!connected || ipAddr != newIp) {
        ipAddr = newIp;
        TYRA_LOG("| IP assigned: ", ipAddr.c_str(), "               |");
      }
      connected      = true;
      initRetryCount = 0;
    } else {
      ipAddr    = g_settings.eth_dhcp ? "Searching (DHCP)..." : "No IP Assigned";
      connected = false;
    }
  } else {
    if (initRetryCount < 300) {
      ipAddr = "Initializing stack...";
      initRetryCount++;
    } else {
      ipAddr = "Stack Error (no IF)";
    }
    connected = false;
  }
}

// ---------------------------------------------------------------------------
// installExceptionHandler
// ---------------------------------------------------------------------------

void NetworkService::installExceptionHandler() {
  ExceptionHandler::install();
}

// ---------------------------------------------------------------------------
// testConnection
// ---------------------------------------------------------------------------

bool NetworkService::testConnection() {
  if (!initialized) {
    lastTestResult = "Service not initialized";
    return false;
  }

  TYRA_LOG("Starting Network Connection Test...");

  TYRA_LOG("Checking Ethernet link...");
  int linkStatus =
      NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0);
  if (linkStatus != NETMAN_NETIF_ETH_LINK_STATE_UP) {
    TYRA_LOG("Warning: Physical link not detecting UP state yet...");
  } else {
    TYRA_LOG("Ethernet link detected as UP.");
  }

  t_ip_info ipInfo;
  if (ps2ip_getconfig((char*)NETIF_NAME, &ipInfo) < 1 || ipInfo.ipaddr.s_addr == 0) {
    if (initRetryCount < 300) {
      lastTestResult = "Stack still initializing (Please wait and test again)";
      TYRA_LOG("Warning: Network stack is currently resolving IP in the "
               "background. Please test again in a few seconds.");
    } else {
      lastTestResult =
          g_settings.eth_dhcp ? "DHCP not ready/Timeout"
                               : "IP not assigned (Static bind failed)";
      TYRA_ERROR("Test Failed: ", lastTestResult.c_str());
    }
    return false;
  }

  TYRA_LOG("IP Address verified: ",
           (u8)(ipInfo.ipaddr.s_addr & 0xFF), ".",
           (u8)((ipInfo.ipaddr.s_addr >> 8) & 0xFF), "...");

  u32 gwAddr = ipInfo.gw.s_addr;
  if (gwAddr == 0) {
    lastTestResult = "No gateway configured";
    TYRA_ERROR("Test Failed: ", lastTestResult.c_str());
    return false;
  }

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    lastTestResult = "Failed to create socket";
    TYRA_ERROR("Test Failed: ", lastTestResult.c_str());
    return false;
  }

  struct sockaddr_in target;
  memset(&target, 0, sizeof(target));
  target.sin_family      = AF_INET;
  target.sin_port        = htons(18194);
  target.sin_addr.s_addr = gwAddr;

  const char* msg  = "TyraCraft Connection Test Heartbeat";
  int         sent = sendto(sock, msg, strlen(msg), 0,
                    (struct sockaddr*)&target, sizeof(target));
  lwip_close(sock);

  if (sent < 0) {
    lastTestResult = "Failed to send data (routing issue?)";
    TYRA_ERROR("Test Failed: ", lastTestResult.c_str());
    return false;
  }

  lastTestResult = "Success (Link + IP + UDP OK)";
  TYRA_LOG("Network Test PASSED: ", lastTestResult.c_str());
  return true;
}

}  // namespace TyraCraft
