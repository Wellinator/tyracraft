#pragma once

#include <stddef.h>

// Based on
// https://github.com/DanielSant0s/AthenaEnv/blob/main/src/include/memory.h
// Always-on after CLNUP-01 (Phase 1) — overhead is negligible (single
// comparison + atomic increment per allocation) and the regression guard
// for RAM only works if measurement is available in any build mode.

extern char __start;
extern char _end;

void init_memory_manager();

size_t get_binary_size();
size_t get_allocs_size();
size_t get_stack_size();
size_t get_used_memory();

// Phase 1 additions (PROF-02 — peakSince(mark) stack-of-marks):
void push_memory_mark();      // pushes a mark; tracks peak from this point
size_t pop_memory_mark();     // pops; returns peak observed during span
