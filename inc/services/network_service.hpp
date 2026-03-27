#pragma once

#include <tamtypes.h>
#include "singleton.hpp"
#include <string>

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

  /** Get the current IP address as string */
  std::string getIpAddress() const { return ipAddr; }

 private:
  bool initialized;
  bool connected;
  std::string ipAddr;

  /** Load required IRX modules for networking. Returns true if all critical modules are loaded. */
  bool loadModules();

  /** Configure network stack (DHCP) */
  bool setupNetwork();

  /** Helper to load IRX modules */
  static int loadIrx(const char* filename);
};

}  // namespace TyraCraft
