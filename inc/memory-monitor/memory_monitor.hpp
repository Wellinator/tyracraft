#pragma once

#ifdef DEBUG_MODE
// Based on
// https://github.com/DanielSant0s/AthenaEnv/blob/main/src/include/memory.h Thx
// Daniel Santos!

extern char __start;
extern char _end;

void init_memory_manager();

size_t get_binary_size();
size_t get_allocs_size();
size_t get_stack_size();
size_t get_used_memory();

#endif  // end if DEBUG_MODE