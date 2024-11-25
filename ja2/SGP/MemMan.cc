// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

//	Purpose :	function definitions for the memory manager
//
// Modification history :
//
//		11sep96:HJH	- Creation
//    29may97:ARM - Fix & improve MemDebugCounter handling, logging of
//                    MemAlloc/MemFree, and reporting of any errors
#include "SGP/MemMan.h"

#include <new>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>

#include "MessageBoxScreen.h"
#include "MouseSystem.h"
#include "SGP/Debug.h"
#include "SGP/Types.h"

static const wchar_t *const gzJA2ScreenNames[] = {
    L"EDIT_SCREEN",
    L"?",
    L"?",
    L"ERROR_SCREEN",
    L"INIT_SCREEN",
    L"GAME_SCREEN",
    L"ANIEDIT_SCREEN",
    L"PALEDIT_SCREEN",
    L"DEBUG_SCREEN",
    L"MAP_SCREEN",
    L"LAPTOP_SCREEN",
    L"LOADSAVE_SCREEN",
    L"MAPUTILITY_SCREEN",
    L"FADE_SCREEN",
    L"MSG_BOX_SCREEN",
    L"MAINMENU_SCREEN",
    L"AUTORESOLVE_SCREEN",
    L"SAVE_LOAD_SCREEN",
    L"OPTIONS_SCREEN",
    L"SHOPKEEPER_SCREEN",
    L"SEX_SCREEN",
    L"GAME_INIT_OPTIONS_SCREEN",
    L"DEMO_EXIT_SCREEN",
    L"INTRO_SCREEN",
    L"CREDIT_SCREEN",
};

// debug variable for total memory currently allocated
static size_t guiMemTotal = 0;
static size_t guiMemAlloced = 0;
static size_t guiMemFreed = 0;
static uint32_t MemDebugCounter = 0;
static BOOLEAN fMemManagerInit = FALSE;

void InitializeMemoryManager() {
  MemDebugCounter = 0;
  guiMemTotal = 0;
  guiMemAlloced = 0;
  guiMemFreed = 0;
  fMemManagerInit = TRUE;
}

// Shuts down the memory manager.
void ShutdownMemoryManager() {
  if (MemDebugCounter != 0) {
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, " ");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, "***** WARNING - WARNING - WARNING *****");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, "***** WARNING - WARNING - WARNING *****");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, "***** WARNING - WARNING - WARNING *****");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, " ");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, "  >>>>> MEMORY LEAK DETECTED!!! <<<<<  ");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0,
             String("%d memory blocks still allocated", MemDebugCounter));
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0,
             String("%d bytes memory total STILL allocated", guiMemTotal));
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0,
             String("%d bytes memory total was allocated", guiMemAlloced));
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0,
             String("%d bytes memory total was freed", guiMemFreed));

    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, " ");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, "***** WARNING - WARNING - WARNING *****");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, "***** WARNING - WARNING - WARNING *****");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, "***** WARNING - WARNING - WARNING *****");
    DebugMsg(TOPIC_MEMORY_MANAGER, DBG_LEVEL_0, " ");
  }

  fMemManagerInit = FALSE;
}

void *XMalloc(size_t const size) {
  void *const p = malloc(size);
  if (!p) throw std::bad_alloc();
  return p;
}

void *XRealloc(void *const ptr, size_t const size) {
  void *const p = realloc(ptr, size);
  if (!p) throw std::bad_alloc();
  return p;
}

void *MallocZ(const size_t n) {
  void *const p = MemAlloc(n);
  memset(p, 0, n);
  return p;
}
