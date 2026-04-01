#include "services/exception_handler.hpp"
#include <debug.h>
#include <ee_debug.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

namespace TyraCraft {

char ExceptionHandler::crashLogPath[256] = "";

void ExceptionHandler::install() {
#ifdef DEBUG_MODE
  // Set default path if none provided (relative to ELF)
  strncpy(crashLogPath, "crash.log", sizeof(crashLogPath) - 1);

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
  ee_dbg_set_level1_handler(15, onException); // Floating Point Exception
  
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
  // Initialize and clear debug screen
  init_scr();
  scr_setbgcolor(0x0000FF); // Red Screen (BGR format)
  scr_clear();

  scr_printf("****************************************************************\n");
  scr_printf("*                       FATAL EXCEPTION                        *\n");
  scr_printf("****************************************************************\n\n");
  
  scr_printf("Cause: %08X   EPC: %08X\n", frame->cause, frame->epc);
  scr_printf("BadV:  %08X   Status: %08X\n", frame->badvaddr, frame->status);
  scr_printf("HI:    %08X   LO:     %08X\n", frame->hi, frame->lo);

  u32 instr = 0;
  if (frame->epc >= 0x00100000 && frame->epc < 0x02000000 && (frame->epc % 4 == 0)) {
    instr = *(u32*)frame->epc;
    scr_printf("Instr: %08X\n\n", instr);
  } else {
    scr_printf("Instr: [INVALID EPC]\n\n");
  }
  
  scr_printf("GPR Dump (Low 32-bit):\n");
  scr_printf("----------------------------------------------------------------\n");
  scr_printf("zr:%08X  at:%08X  v0:%08X  v1:%08X\n", 0, frame->at[0], frame->v0[0], frame->v1[0]);
  scr_printf("a0:%08X  a1:%08X  a2:%08X  a3:%08X\n", frame->a0[0], frame->a1[0], frame->a2[0], frame->a3[0]);
  scr_printf("t0:%08X  t1:%08X  t2:%08X  t3:%08X\n", frame->t0[0], frame->t1[0], frame->t2[0], frame->t3[0]);
  scr_printf("t4:%08X  t5:%08X  t6:%08X  t7:%08X\n", frame->t4[0], frame->t5[0], frame->t6[0], frame->t7[0]);
  scr_printf("s0:%08X  s1:%08X  s2:%08X  s3:%08X\n", frame->s0[0], frame->s1[0], frame->s2[0], frame->s3[0]);
  scr_printf("s4:%08X  s5:%08X  s6:%08X  s7:%08X\n", frame->s4[0], frame->s5[0], frame->s6[0], frame->s7[0]);
  scr_printf("t8:%08X  t9:%08X  k0:%08X  k1:%08X\n", frame->t8[0], frame->t9[0], frame->k0[0], frame->k1[0]);
  scr_printf("gp:%08X  sp:%08X  fp:%08X  ra:%08X\n", frame->gp[0], frame->sp[0], frame->fp[0], frame->ra[0]);
  scr_printf("----------------------------------------------------------------\n\n");
  
  scr_printf("The system has halted. Please record info and restart.\n");
  
  // Save to RAM for next boot recovery
  saveCrashDump(frame);

  while(1);
#endif
  return 0;
}

void ExceptionHandler::saveCrashDump(struct st_EE_RegFrame* frame) {
  // Use UNCATCHED segment (KSEG1) to ensure RDRAM is written immediately
  CrashDump* dump = (CrashDump*)(0xA0000000 | CRASH_DUMP_ADDR);
  
  dump->magic = CRASH_DUMP_MAGIC;
  dump->cause = frame->cause;
  dump->epc   = frame->epc;
  dump->badvaddr = frame->badvaddr;
  dump->status = frame->status;
  dump->lo = frame->lo;
  dump->hi = frame->hi;
  
  // Safe instruction read
  if (frame->epc >= 0x00100000 && frame->epc < 0x02000000 && (frame->epc % 4 == 0)) {
     dump->instr = *(u32*)frame->epc;
  } else {
     dump->instr = 0xDEADBEEF;
  }

  // Save all GPRs
  dump->gpr[0] = 0;
  dump->gpr[1] = frame->at[0];
  dump->gpr[2] = frame->v0[0];
  dump->gpr[3] = frame->v1[0];
  dump->gpr[4] = frame->a0[0];
  dump->gpr[5] = frame->a1[0];
  dump->gpr[6] = frame->a2[0];
  dump->gpr[7] = frame->a3[0];
  dump->gpr[8] = frame->t0[0];
  dump->gpr[9] = frame->t1[0];
  dump->gpr[10] = frame->t2[0];
  dump->gpr[11] = frame->t3[0];
  dump->gpr[12] = frame->t4[0];
  dump->gpr[13] = frame->t5[0];
  dump->gpr[14] = frame->t6[0];
  dump->gpr[15] = frame->t7[0];
  dump->gpr[16] = frame->s0[0];
  dump->gpr[17] = frame->s1[0];
  dump->gpr[18] = frame->s2[0];
  dump->gpr[19] = frame->s3[0];
  dump->gpr[20] = frame->s4[0];
  dump->gpr[21] = frame->s5[0];
  dump->gpr[22] = frame->s6[0];
  dump->gpr[23] = frame->s7[0];
  dump->gpr[24] = frame->t8[0];
  dump->gpr[25] = frame->t9[0];
  dump->gpr[26] = frame->k0[0];
  dump->gpr[27] = frame->k1[0];
  dump->gpr[28] = frame->gp[0];
  dump->gpr[29] = frame->sp[0];
  dump->gpr[30] = frame->fp[0];
  dump->gpr[31] = frame->ra[0];
  
  scr_printf("\nCrash info saved to RAM. System Halted.\n");
}

bool ExceptionHandler::hasStoredDump() {
  CrashDump* dump = (CrashDump*)(0xA0000000 | CRASH_DUMP_ADDR);
  return dump->magic == CRASH_DUMP_MAGIC;
}

void ExceptionHandler::handleStoredDump() {
  if (!hasStoredDump()) return;
  
  CrashDump* dump = (CrashDump*)(0xA0000000 | CRASH_DUMP_ADDR);
  
  // Now that we are in a stable environment (game init), we write to file
  int fd = open("crash.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    printf("[ExceptionHandler] Failed to save deferred crash log!\n");
    // Clear magic anyway as we can't do much
    dump->magic = 0;
    return;
  }
  
  char buf[1024];
  int len = snprintf(buf, sizeof(buf),
    "TyraCraft RECOVERY EXCEPTION REPORT (v2)\n"
    "------------------------------------\n"
    "This crash occurred in the PREVIOUS session.\n\n"
    "Cause: %08X   EPC:   %08X\n"
    "BadV:  %08X   Status: %08X\n"
    "HI:    %08X   LO:     %08X\n"
    "Instr: %08X\n\n"
    "GPR Dump (Low 32-bit):\n"
    "zr:%08X  at:%08X  v0:%08X  v1:%08X\n"
    "a0:%08X  a1:%08X  a2:%08X  a3:%08X\n"
    "t0:%08X  t1:%08X  t2:%08X  t3:%08X\n"
    "t4:%08X  t5:%08X  t6:%08X  t7:%08X\n"
    "s0:%08X  s1:%08X  s2:%08X  s3:%08X\n"
    "s4:%08X  s5:%08X  s6:%08X  s7:%08X\n"
    "t8:%08X  t9:%08X  k0:%08X  k1:%08X\n"
    "gp:%08X  sp:%08X  fp:%08X  ra:%08X\n"
    "------------------------------------\n",
    dump->cause, dump->epc, dump->badvaddr, dump->status,
    dump->hi, dump->lo, dump->instr,
    dump->gpr[0], dump->gpr[1], dump->gpr[2], dump->gpr[3],
    dump->gpr[4], dump->gpr[5], dump->gpr[6], dump->gpr[7],
    dump->gpr[8], dump->gpr[9], dump->gpr[10], dump->gpr[11],
    dump->gpr[12], dump->gpr[13], dump->gpr[14], dump->gpr[15],
    dump->gpr[16], dump->gpr[17], dump->gpr[18], dump->gpr[19],
    dump->gpr[20], dump->gpr[21], dump->gpr[22], dump->gpr[23],
    dump->gpr[24], dump->gpr[25], dump->gpr[26], dump->gpr[27],
    dump->gpr[28], dump->gpr[29], dump->gpr[30], dump->gpr[31]
  );
  
  if (len > 0) {
    write(fd, buf, len);
    printf("[ExceptionHandler] Previous crash log saved to crash.log\n");
  }
  
  close(fd);
  
  // Clear Magic to avoid repeated logging
  dump->magic = 0;
}

}  // namespace TyraCraft
