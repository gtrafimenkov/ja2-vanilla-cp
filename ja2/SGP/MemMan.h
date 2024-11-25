// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

// Purpose: prototypes for the memory manager
//
// Modification history :
//		11sep96:HJH				- Creation

#ifndef _MEMMAN_H
#define _MEMMAN_H

#include "SGP/Types.h"

void InitializeMemoryManager();
void ShutdownMemoryManager();

// Creates and adds a video object to list
// Release build version
#include <stdlib.h>
#define MemAlloc(size) XMalloc((size))
#define MemFree(ptr) free((ptr))
#define MemRealloc(ptr, size) XRealloc((ptr), (size))
void *XMalloc(size_t size);
void *XRealloc(void *ptr, size_t size);

void *MallocZ(const size_t n);

template <typename T>
static inline void FreeNull(T *&r) throw() {
  T *const p = r;
  if (!p) return;
  r = 0;
  MemFree(p);
}

#define MALLOC(type) (type *)MemAlloc(sizeof(type))
#define MALLOCE(type, member, n) (type *)MemAlloc(sizeof(type) + sizeof(*((type *)0)->member) * (n))
#define MALLOCN(type, count) (type *)MemAlloc(sizeof(type) * (count))
#define MALLOCNZ(type, count) (type *)MallocZ(sizeof(type) * (count))
#define MALLOCZ(type) (type *)MallocZ(sizeof(type))

#define REALLOC(ptr, type, count) (type *)MemRealloc(ptr, sizeof(type) * (count))

#endif
