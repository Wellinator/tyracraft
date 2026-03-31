#pragma once

#include <tamtypes.h>
#include "singleton.hpp"
#include <string>
#include "timer.hpp"

// Include PS2IP types to avoid incomplete type errors in C++
extern "C" {
#include <netman.h>
#include <ps2ip.h>
}

namespace TyraCraft {

class NetworkService : public Singleton<NetworkService> {
 public:
  NetworkService();
  ~NetworkService();

  /**
   * Initialize network modules and stack.
   * Returns true if initialization was successful.
   */
  bool init();

  /** Check if the network is initialized and has an IP */
  bool isConnected() const { return connected; }

  /** Refresh DHCP/IP status — call every frame */
  void update();

  /** Install the EE Exception Handler (Trap) */
  void installExceptionHandler();

  /** Get the current IP address as string */
  std::string getIpAddress() const { return ipAddr; }

  /**
   * Performs a real-world connectivity test by checking link, IP,
   * and attempting to reach the gateway or a target host.
   * isSilent=true suppresses progress logs (useful for auto-retries).
   */
  bool testConnection(bool isSilent = false);

  /** Get result of last test */
  std::string getLastTestResult() const { return lastTestResult; }

  /** Send a raw buffer via UDP to the log server (used by _write hook) */
  void sendRemoteLogRaw(const void* buf, size_t len);

  /** Check if the system is ready to transmit logs over network */
  bool isReadyForLogging() const {
    return connected && testPassed && g_settings.enable_log_over_lan;
  }

 private:
  bool initialized;
  bool connected;
  bool testPassed;
  std::string ipAddr;
  std::string lastTestResult;
  u32 initRetryCount;
  Timer::ElapsedTimer retryTimer;

  // Remote log destination
  u32 logTargetIp;
  s32 networkMutex;
  int loggerSocket;

  // lwIP interface name: SMAP always registers as "sm0" (name[0]='s', name[1]='m', num=0).
  // This is distinct from the NetMan-level name ("SMAP").  The official ps2sdk samples
  // all hardcode "sm0"; we follow that convention.
  static const char* const NETIF_NAME;  // = "sm0"

  /** Load required IRX modules for networking. Returns true if all critical modules loaded. */
  bool loadModules();

  /** Individual IRX loading functions */
  bool loadPs2Dev9(const std::string& irxDir);
  bool loadNetman(const std::string& irxDir);
  bool loadSmap(const std::string& irxDir);
  bool loadUdptty(const std::string& irxDir);

  /** Configure network stack following the official tcpip_dhcp / tcpip_basic samples */
  bool setupNetwork();

  /**
   * Mirrors the official ps2sdk ethApplyNetIFConfig().
   * Sets the link auto-negotiation mode via NetManSetLinkMode().
   */
  int ethApplyNetIFConfig(int mode);

  /**
   * Mirrors the official ps2sdk ethApplyIPConfig().
   * use_dhcp=1 → starts DHCP client; use_dhcp=0 → sets static IP.
   */
  int ethApplyIPConfig(int use_dhcp,
                       const ::ip4_addr* ip,
                       const ::ip4_addr* netmask,
                       const ::ip4_addr* gateway);

  /** Helper to load IRX modules (with optional arguments) */
  static int loadIrx(const char* filename, int argc = 0, char** argv = NULL);

  /** Optimized and thread-safe remote logging initialization */
  void initLoggerSocket();
  void closeLoggerSocket();
};

}  // namespace TyraCraft
