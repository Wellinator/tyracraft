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
};

}  // namespace TyraCraft
