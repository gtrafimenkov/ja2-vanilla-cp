#ifndef CURSOR_CONTROL_H
#define CURSOR_CONTROL_H

#include "SGP/Types.h"

void CursorDatabaseClear();
BOOLEAN SetCurrentCursorFromDatabase(uint32_t uiCursorIndex);

#define USE_OUTLINE_BLITTER 0x08

#define EXTERN_CURSOR 0xFFF0
#define MAX_COMPOSITES 5
#define CENTER_SUBCURSOR 31000
#define HIDE_SUBCURSOR 32000

#define CENTER_CURSOR 32000
#define RIGHT_CURSOR 32001
#define LEFT_CURSOR 32002
#define TOP_CURSOR 32003
#define BOTTOM_CURSOR 32004

#define CURSOR_TO_FLASH 0x01
#define CURSOR_TO_FLASH2 0x02
#define CURSOR_TO_SUB_CONDITIONALLY 0x04
#define DELAY_START_CURSOR 0x08
#define CURSOR_TO_PLAY_SOUND 0x10

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Cursor Database
//
///////////////////////////////////////////////////////////////////////////////////////////////////

struct CursorFileData {
  const char *Filename;  // If NULL then an extern video object is used
  HVOBJECT hVObject;
  uint8_t ubFlags;
  uint8_t ubNumberOfFrames;  // If != 0 then the cursor is animated
};

struct CursorImage {
  uint32_t uiFileIndex;
  uint16_t uiSubIndex;
  uint32_t uiCurrentFrame;
  int16_t usPosX;
  int16_t usPosY;
};

struct CursorData {
  CursorImage Composites[MAX_COMPOSITES];
  uint16_t usNumComposites;
  int16_t sOffsetX;
  int16_t sOffsetY;
  uint16_t usHeight;
  uint16_t usWidth;
  uint8_t bFlags;
  uint8_t bFlashIndex;
};

extern int16_t gsGlobalCursorYOffset;

// Globals for cursor database offset values
extern uint16_t gsCurMouseHeight;
extern uint16_t gsCurMouseWidth;

void SetExternMouseCursor(SGPVObject const &, uint16_t region_idx);

typedef void (*MOUSEBLT_HOOK)();

void InitCursorDatabase(CursorFileData *pCursorFileData, CursorData *pCursorData,
                        uint16_t suNumDataFiles);
void SetMouseBltHook(MOUSEBLT_HOOK pMouseBltOverride);

void SetExternVOData(uint32_t uiCursorIndex, HVOBJECT hVObject, uint16_t usSubIndex);

#endif
