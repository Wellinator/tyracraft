
#include <reent.h>
#include <kernel.h>
#include <malloc.h>
#include <stdbool.h>
#include <tyra>
#include "memory-monitor/memory_monitor.hpp"

typedef struct {
  size_t binary_size;
  size_t allocs_size;
  size_t stack_size;
} MemoryMonitor;

static MemoryMonitor prog_mem;

struct Mark {
  size_t startUsed;
  size_t peakUsed;
};
static constexpr size_t MAX_MARKS = 16;
static Mark s_markStack[MAX_MARKS];
static size_t s_markDepth = 0;

static inline void sample_peak_into_active_marks() {
  if (s_markDepth == 0) return;          // early-out — most allocs pay nothing
  const size_t cur = prog_mem.stack_size + prog_mem.allocs_size + prog_mem.binary_size;
  for (size_t i = 0; i < s_markDepth; ++i) {
    if (cur > s_markStack[i].peakUsed) {
      s_markStack[i].peakUsed = cur;
    }
  }
}

void push_memory_mark() {
  if (s_markDepth >= MAX_MARKS) {
    TYRA_TRAP("MemoryMonitor: mark stack overflow");
    return;
  }
  const size_t cur = get_used_memory();
  s_markStack[s_markDepth].startUsed = cur;
  s_markStack[s_markDepth].peakUsed = cur;
  s_markDepth++;
}

size_t pop_memory_mark() {
  if (s_markDepth == 0) {
    TYRA_TRAP("MemoryMonitor: pop without matching push");
    return 0;
  }
  s_markDepth--;
  return s_markStack[s_markDepth].peakUsed;
}

void* malloc(size_t size) {
  size_t* ptr = reinterpret_cast<size_t*>(_malloc_r(_REENT, size));

  if (ptr) {
    prog_mem.allocs_size += ((size_t*)ptr)[-1];
    sample_peak_into_active_marks();
  }

  return ptr;
}

void* realloc(void* memblock, size_t size) {
  if (memblock) {
    prog_mem.allocs_size -= ((size_t*)memblock)[-1];
    sample_peak_into_active_marks();
  }

  size_t* ptr = reinterpret_cast<size_t*>(_realloc_r(_REENT, memblock, size));

  if (ptr) {
    prog_mem.allocs_size += ((size_t*)ptr)[-1];
    sample_peak_into_active_marks();
  }

  return ptr;
}

void* calloc(size_t number, size_t size) {
  size_t* ptr = reinterpret_cast<size_t*>(_calloc_r(_REENT, number, size));

  if (ptr) {
    prog_mem.allocs_size += ((size_t*)ptr)[-1];
    sample_peak_into_active_marks();
  }

  return ptr;
}

void* memalign(size_t alignment, size_t size) {
  size_t* ptr = reinterpret_cast<size_t*>(_memalign_r(_REENT, alignment, size));

  if (ptr) {
    prog_mem.allocs_size += ((size_t*)ptr)[-1];
    sample_peak_into_active_marks();
  }

  return ptr;
}

void free(void* ptr) {
  if (ptr) {
    prog_mem.allocs_size -= ((size_t*)ptr)[-1];
    sample_peak_into_active_marks();
  }

  _free_r(_REENT, ptr);
}

void init_memory_manager() {
  prog_mem.binary_size = (unsigned long)&_end - (unsigned long)&__start;
  prog_mem.stack_size = 0x20000;
}

size_t get_binary_size() { return prog_mem.binary_size; }
size_t get_allocs_size() { return prog_mem.allocs_size; }
size_t get_stack_size() { return prog_mem.stack_size; }
size_t get_used_memory() {
  return prog_mem.stack_size + prog_mem.allocs_size + prog_mem.binary_size;
}
