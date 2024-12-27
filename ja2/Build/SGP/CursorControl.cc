// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/CursorControl.h"

#include "SGP/Debug.h"
#include "SGP/HImage.h"
#include "SGP/Timer.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Cursor Database
//
///////////////////////////////////////////////////////////////////////////////////////////////////

static BOOLEAN gfCursorDatabaseInit = FALSE;

static CursorFileData *gpCursorFileDatabase;
static CursorData *gpCursorDatabase;
int16_t gsGlobalCursorYOffset = 0;
uint16_t gsCurMouseHeight = 0;
uint16_t gsCurMouseWidth = 0;
static uint16_t gusNumDataFiles = 0;
static SGPVObject const *guiExternVo;
static uint16_t gusExternVoSubIndex;
static uint32_t guiOldSetCursor = 0;
static uint32_t guiDelayTimer = 0;

static MOUSEBLT_HOOK gMouseBltOverride = NULL;

static void EraseMouseCursor() { MOUSE_BUFFER->Fill(0); }

static void BltToMouseCursorFromVObject(HVOBJECT hVObject, uint16_t usVideoObjectSubIndex,
                                        uint16_t usXPos, uint16_t usYPos) {
  BltVideoObject(MOUSE_BUFFER, hVObject, usVideoObjectSubIndex, usXPos, usYPos);
}

static void BltToMouseCursorFromVObjectWithOutline(HVOBJECT hVObject,
                                                   uint16_t usVideoObjectSubIndex, uint16_t usXPos,
                                                   uint16_t usYPos) {
  // Center and adjust for offsets
  ETRLEObject const &pTrav = hVObject->SubregionProperties(usVideoObjectSubIndex);
  int16_t const sXPos = (gsCurMouseWidth - pTrav.usWidth) / 2 - pTrav.sOffsetX;
  int16_t const sYPos = (gsCurMouseHeight - pTrav.usHeight) / 2 - pTrav.sOffsetY;
  BltVideoObjectOutline(MOUSE_BUFFER, hVObject, usVideoObjectSubIndex, sXPos, sYPos,
                        Get16BPPColor(FROMRGB(0, 255, 0)));
}

// THESE TWO PARAMETERS MUST POINT TO STATIC OR GLOBAL DATA, NOT AUTOMATIC
// VARIABLES
void InitCursorDatabase(CursorFileData *pCursorFileData, CursorData *pCursorData,
                        uint16_t suNumDataFiles) {
  // Set global values!
  gpCursorFileDatabase = pCursorFileData;
  gpCursorDatabase = pCursorData;
  gusNumDataFiles = suNumDataFiles;
  gfCursorDatabaseInit = TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Cursor Handlers
//
///////////////////////////////////////////////////////////////////////////////////////////////////

static void LoadCursorData(uint32_t uiCursorIndex) {
  // Load cursor data will load all data required for the cursor specified by
  // this index
  CursorData *pCurData = &gpCursorDatabase[uiCursorIndex];

  int16_t sMaxHeight = -1;
  int16_t sMaxWidth = -1;
  for (uint32_t cnt = 0; cnt < pCurData->usNumComposites; cnt++) {
    const CursorImage *pCurImage = &pCurData->Composites[cnt];
    CursorFileData *CFData = &gpCursorFileDatabase[pCurImage->uiFileIndex];

    if (CFData->hVObject == NULL) {
      // The file containing the video object hasn't been loaded yet. Let's load
      // it now First load as an SGPImage so we can get aux data!
      Assert(CFData->Filename != NULL);

      AutoSGPImage hImage(CreateImage(CFData->Filename, IMAGE_ALLDATA));

      CFData->hVObject = AddVideoObjectFromHImage(hImage);

      // Check for animated tile
      if (hImage->uiAppDataSize > 0) {
        // Valid auxiliary data, so get # od frames from data
        AuxObjectData const *const pAuxData =
            (AuxObjectData const *)(uint8_t const *)hImage->pAppData;
        if (pAuxData->fFlags & AUX_ANIMATED_TILE) {
          CFData->ubNumberOfFrames = pAuxData->ubNumberOfFrames;
        }
      }
    }

    // Get ETRLE Data for this video object
    ETRLEObject const &pTrav = CFData->hVObject->SubregionProperties(pCurImage->uiSubIndex);

    if (pTrav.usHeight > sMaxHeight) sMaxHeight = pTrav.usHeight;
    if (pTrav.usWidth > sMaxWidth) sMaxWidth = pTrav.usWidth;
  }

  pCurData->usHeight = sMaxHeight;
  pCurData->usWidth = sMaxWidth;

  switch (pCurData->sOffsetX) {
    case CENTER_CURSOR:
      pCurData->sOffsetX = pCurData->usWidth / 2;
      break;
    case RIGHT_CURSOR:
      pCurData->sOffsetX = pCurData->usWidth;
      break;
    case LEFT_CURSOR:
      pCurData->sOffsetX = 0;
      break;
  }

  switch (pCurData->sOffsetY) {
    case CENTER_CURSOR:
      pCurData->sOffsetY = pCurData->usHeight / 2;
      break;
    case BOTTOM_CURSOR:
      pCurData->sOffsetY = pCurData->usHeight;
      break;
    case TOP_CURSOR:
      pCurData->sOffsetY = 0;
      break;
  }

  gsCurMouseHeight = pCurData->usHeight;
  gsCurMouseWidth = pCurData->usWidth;

  // Adjust relative offsets
  for (uint32_t cnt = 0; cnt < pCurData->usNumComposites; cnt++) {
    CursorImage *pCurImage = &pCurData->Composites[cnt];

    // Get ETRLE Data for this video object
    ETRLEObject const &pTrav =
        gpCursorFileDatabase[pCurImage->uiFileIndex].hVObject->SubregionProperties(
            pCurImage->uiSubIndex);

    if (pCurImage->usPosX == CENTER_SUBCURSOR) {
      pCurImage->usPosX = pCurData->sOffsetX - pTrav.usWidth / 2;
    }

    if (pCurImage->usPosY == CENTER_SUBCURSOR) {
      pCurImage->usPosY = pCurData->sOffsetY - pTrav.usHeight / 2;
    }
  }
}

static void UnLoadCursorData(uint32_t uiCursorIndex) {
  // This function will unload add data used for this cursor
  //
  // Ok, first we make sure that the video object file is indeed loaded. Once
  // this is verified, we will move on to the deletion
  const CursorData *pCurData = &gpCursorDatabase[uiCursorIndex];

  for (uint32_t cnt = 0; cnt < pCurData->usNumComposites; cnt++) {
    const CursorImage *pCurImage = &pCurData->Composites[cnt];
    CursorFileData *CFData = &gpCursorFileDatabase[pCurImage->uiFileIndex];

    if (CFData->hVObject != NULL && CFData->Filename != NULL) {
      DeleteVideoObject(CFData->hVObject);
      CFData->hVObject = NULL;
    }
  }
}

void CursorDatabaseClear() {
  for (uint32_t uiIndex = 0; uiIndex < gusNumDataFiles; uiIndex++) {
    CursorFileData *CFData = &gpCursorFileDatabase[uiIndex];
    if (CFData->hVObject != NULL && CFData->Filename != NULL) {
      DeleteVideoObject(CFData->hVObject);
      CFData->hVObject = NULL;
    }
  }
}

BOOLEAN SetCurrentCursorFromDatabase(uint32_t uiCursorIndex) {
  if (uiCursorIndex == VIDEO_NO_CURSOR) {
    SetMouseCursorProperties(0, 0, 0, 0);
  } else if (gfCursorDatabaseInit) {
    // CHECK FOR EXTERN CURSOR
    if (uiCursorIndex == EXTERN_CURSOR) {
      // Erase old cursor
      EraseMouseCursor();

      ETRLEObject const &pTrav = guiExternVo->SubregionProperties(gusExternVoSubIndex);
      uint16_t const usEffHeight = pTrav.usHeight + pTrav.sOffsetY;
      uint16_t const usEffWidth = pTrav.usWidth + pTrav.sOffsetX;

      BltVideoObjectOutline(MOUSE_BUFFER, guiExternVo, gusExternVoSubIndex, 0, 0, SGP_TRANSPARENT);

      // Hook into hook function
      if (gMouseBltOverride != NULL) gMouseBltOverride();

      SetMouseCursorProperties(usEffWidth / 2, usEffHeight / 2, usEffHeight, usEffWidth);
    } else {
      const CursorData *pCurData = &gpCursorDatabase[uiCursorIndex];

      // First check if we are a differnet curosr...
      if (uiCursorIndex != guiOldSetCursor) {
        // OK, check if we are a delay cursor...
        if (pCurData->bFlags & DELAY_START_CURSOR) {
          guiDelayTimer = GetClock();
        }
      }

      guiOldSetCursor = uiCursorIndex;

      // Olny update if delay timer has elapsed...
      if (pCurData->bFlags & DELAY_START_CURSOR) {
        if (GetClock() - guiDelayTimer < 1000) {
          SetMouseCursorProperties(0, 0, 0, 0);
          return TRUE;
        }
      }

      // Call LoadCursorData to make sure that the video object is loaded
      LoadCursorData(uiCursorIndex);

      // Erase old cursor
      EraseMouseCursor();
      // NOW ACCOMODATE COMPOSITE CURSORS
      pCurData = &gpCursorDatabase[uiCursorIndex];

      for (uint32_t cnt = 0; cnt < pCurData->usNumComposites; cnt++) {
        // Check if we are a flashing cursor!
        if (pCurData->bFlags & CURSOR_TO_FLASH && cnt <= 1 && pCurData->bFlashIndex != cnt) {
          continue;
        }
        // Check if we are a sub cursor!
        // IN this case, do all frames but
        // skip the 1st or second!

        if (pCurData->bFlags & CURSOR_TO_SUB_CONDITIONALLY) {
          if (pCurData->bFlags & CURSOR_TO_FLASH2) {
            if (0 < cnt && cnt <= 2 && pCurData->bFlashIndex != cnt) {
              continue;
            }
          } else {
            if (cnt <= 1 && pCurData->bFlashIndex != cnt) {
              continue;
            }
          }
        }

        const CursorImage *pCurImage = &pCurData->Composites[cnt];
        CursorFileData *CFData = &gpCursorFileDatabase[pCurImage->uiFileIndex];

        // Adjust sub-index if cursor is animated
        uint16_t usSubIndex;
        if (CFData->ubNumberOfFrames != 0) {
          usSubIndex = pCurImage->uiCurrentFrame;
        } else {
          usSubIndex = pCurImage->uiSubIndex;
        }

        if (pCurImage->usPosX != HIDE_SUBCURSOR && pCurImage->usPosY != HIDE_SUBCURSOR) {
          // Blit cursor at position in mouse buffer
          if (CFData->ubFlags & USE_OUTLINE_BLITTER) {
            BltToMouseCursorFromVObjectWithOutline(CFData->hVObject, usSubIndex, pCurImage->usPosX,
                                                   pCurImage->usPosY);
          } else {
            BltToMouseCursorFromVObject(CFData->hVObject, usSubIndex, pCurImage->usPosX,
                                        pCurImage->usPosY);
          }
        }
      }

      // Hook into hook function
      if (gMouseBltOverride != NULL) gMouseBltOverride();

      int16_t sCenterValX = pCurData->sOffsetX;
      int16_t sCenterValY = pCurData->sOffsetY;
      SetMouseCursorProperties(sCenterValX, sCenterValY + gsGlobalCursorYOffset, pCurData->usHeight,
                               pCurData->usWidth);
    }
  }

  return TRUE;
}

void SetMouseBltHook(MOUSEBLT_HOOK pMouseBltOverride) { gMouseBltOverride = pMouseBltOverride; }

// Sets an external video object as cursor file data....
void SetExternVOData(uint32_t uiCursorIndex, HVOBJECT hVObject, uint16_t usSubIndex) {
  CursorData *pCurData = &gpCursorDatabase[uiCursorIndex];

  for (uint32_t cnt = 0; cnt < pCurData->usNumComposites; cnt++) {
    CursorImage *pCurImage = &pCurData->Composites[cnt];
    CursorFileData *CFData = &gpCursorFileDatabase[pCurImage->uiFileIndex];

    if (CFData->Filename == NULL) {
      // OK, set Video Object here....

      // If loaded, unload...
      UnLoadCursorData(uiCursorIndex);

      // Set extern vo
      CFData->hVObject = hVObject;
      pCurImage->uiSubIndex = usSubIndex;

      // Reload....
      LoadCursorData(uiCursorIndex);
    }
  }
}

void SetExternMouseCursor(SGPVObject const &vo, uint16_t const region_idx) {
  guiExternVo = &vo;
  gusExternVoSubIndex = region_idx;
}
