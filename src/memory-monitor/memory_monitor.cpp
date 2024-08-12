
#ifdef DEBUG_MODE

#include <reent.h>
#include <kernel.h>
#include <malloc.h>
#include <stdbool.h>
#include "memory-monitor/memory_monitor.hpp"

typedef struct {
  size_t binary_size;
  size_t allocs_size;
  size_t stack_size;
} MemoryMonitor;

static MemoryMonitor prog_mem;

void* malloc(size_t size) {
  size_t* ptr = reinterpret_cast<size_t*>(_malloc_r(_REENT, size));

  if (ptr) {
    prog_mem.allocs_size += ((size_t*)ptr)[-1];
  }

  return ptr;
}

void* realloc(void* memblock, size_t size) {
  if (memblock) {
    prog_mem.allocs_size -= ((size_t*)memblock)[-1];
  }

  size_t* ptr = reinterpret_cast<size_t*>(_realloc_r(_REENT, memblock, size));

  if (ptr) {
    prog_mem.allocs_size += ((size_t*)ptr)[-1];
  }

  return ptr;
}

void* calloc(size_t number, size_t size) {
  size_t* ptr = reinterpret_cast<size_t*>(_calloc_r(_REENT, number, size));

  if (ptr) {
    prog_mem.allocs_size += ((size_t*)ptr)[-1];
  }

  return ptr;
}

void* memalign(size_t alignment, size_t size) {
  size_t* ptr = reinterpret_cast<size_t*>(_memalign_r(_REENT, alignment, size));

  if (ptr) {
    prog_mem.allocs_size += ((size_t*)ptr)[-1];
  }

  return ptr;
}

void free(void* ptr) {
  if (ptr) {
    prog_mem.allocs_size -= ((size_t*)ptr)[-1];
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

#endif //end if DEBUG_MODE