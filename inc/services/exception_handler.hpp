#pragma once

#include <ps2_debug.h>

namespace TyraCraft {

/**
 * Capture state of the Emotion Engine during a crash/exception.
 */
class ExceptionHandler {
 public:
  static void install();
  static void uninstall();

  /**
   * Hooked low-level handler called by ee_debug.
   */
  static int onException(struct st_EE_RegFrame* frame);

  /**
   * Deferred logging: Checks if a crash dump exists in RAM.
   */
  static bool hasStoredDump();

  /**
   * Deferred logging: Writes the stored crash dump to a file.
   */
  static void handleStoredDump();

 private:
  static void saveCrashDump(struct st_EE_RegFrame* frame);
  static char crashLogPath[256];

  struct CrashDump {
    u32 magic;
    u32 cause;
    u32 epc;
    u32 badvaddr;
    u32 status;
    u32 lo, hi;
    u32 instr;
    u32 gpr[32]; // All 32-bit low GPRs (zero to ra)
  };

  static const u32 CRASH_DUMP_MAGIC = 0x43525349; // 'CRSI' (Crash Info v2)
  static const u32 CRASH_DUMP_ADDR  = 0x01FE0000;
};

}  // namespace TyraCraft
