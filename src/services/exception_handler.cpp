#include "services/exception_handler.hpp"
#include <debug.h>
#include <ee_debug.h>
#include <stdio.h>

namespace TyraCraft {

void ExceptionHandler::install() {
#ifdef DEBUG_MODE
  ee_dbg_install(1); // Install Level 1 debug handlers
  
  // Hook fatal exceptions
  ee_dbg_set_level1_handler(4, onException);  // Address Error (load)
  ee_dbg_set_level1_handler(5, onException);  // Address Error (store)
  ee_dbg_set_level1_handler(6, onException);  // Bus Error (instruction)
  ee_dbg_set_level1_handler(7, onException);  // Bus Error (data)
  ee_dbg_set_level1_handler(10, onException); // Reserved Instruction
  ee_dbg_set_level1_handler(11, onException); // Coprocessor Unusable
  ee_dbg_set_level1_handler(12, onException); // Arithmetic Overflow
  ee_dbg_set_level1_handler(13, onException); // Trap
  
  printf("Safe EE Exception Handler installed via ee_debug.\n");
#endif
}

void ExceptionHandler::uninstall() {
#ifdef DEBUG_MODE
  ee_dbg_remove(1);
#endif
}

int ExceptionHandler::onException(struct st_EE_RegFrame* frame) {
#ifdef DEBUG_MODE
  // Use PS2SDK standard debug screen
  init_scr();
  scr_setbgcolor(0x0000FF); // Red Screen (BGR format)
  scr_clear();
  
  scr_printf("*********************************\n");
  scr_printf("*       FATAL EXCEPTION         *\n");
  scr_printf("*********************************\n\n");
  
  scr_printf("Cause: %08X   EPC: %08X\n", frame->cause, frame->epc);
  scr_printf("BadV:  %08X   Status: %08X\n\n", frame->badvaddr, frame->status);
  
  scr_printf("GPR Dump (Low 32-bit):\n");
  scr_printf("v0: %08X  v1: %08X  a0: %08X\n", frame->v0[0], frame->v1[0], frame->a0[0]);
  scr_printf("a1: %08X  a2: %08X  a3: %08X\n", frame->a1[0], frame->a2[0], frame->a3[0]);
  scr_printf("t0: %08X  t1: %08X  t2: %08X\n", frame->t0[0], frame->t1[0], frame->t2[0]);
  scr_printf("s0: %08X  s1: %08X  ra: %08X\n", frame->s0[0], frame->s1[0], frame->ra[0]);
  scr_printf("sp: %08X  gp: %08X  fp: %08X\n\n", frame->sp[0], frame->gp[0], frame->fp[0]);
  
  scr_printf("The system has halted.\n");
  scr_printf("Please record the info above and reboot.\n");
  
  while(1);
#endif
  return 0;
}

}  // namespace TyraCraft
