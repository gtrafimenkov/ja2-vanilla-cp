// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Utils/MapUtility.h"

#include "Directories.h"
#include "Editor/LoadScreen.h"
#include "Local.h"
#include "SGP/English.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/Line.h"
#include "SGP/MemMan.h"
#include "SGP/SGP.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Screens.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "TileEngine/OverheadMap.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/WorldDat.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"
#include "Utils/Quantize.h"
#include "Utils/STIConvert.h"

#include "SDL_keycode.h"
#include "SDL_pixels.h"

#define MINIMAP_X_SIZE 88
#define MINIMAP_Y_SIZE 44

#define WINDOW_SIZE 2

static float gdXStep;
static float gdYStep;
static SGPVSurface *giMiniMap;
static SGPVSurface *gi8BitMiniMap;

// Utililty file for sub-sampling/creating our radar screen maps
// Loops though our maps directory and reads all .map files, subsamples an area,
// color quantizes it into an 8-bit image ans writes it to an sti file in
// radarmaps.

ScreenID MapUtilScreenHandle() {
  static SGPPaletteEntry *p24BitValues = NULL;
  static int16_t fNewMap = TRUE;
  InputAtom InputEvent;
  static FDLG_LIST *FListNode;
  static int16_t sFiles = 0, sCurFile = 0;
  static FDLG_LIST *FileList = NULL;

  uint32_t uiRGBColor;

  uint32_t bR, bG, bB, bAvR, bAvG, bAvB;
  int16_t s16BPPSrc, sDest16BPPColor;
  int32_t cnt;

  int16_t sX1, sX2, sY1, sY2, sTop, sBottom, sLeft, sRight;

  float dX, dY, dStartX, dStartY;
  int32_t iX, iY, iSubX1, iSubY1, iSubX2, iSubY2, iWindowX, iWindowY, iCount;
  SGPPaletteEntry pPalette[256];

  sDest16BPPColor = -1;
  bAvR = bAvG = bAvB = 0;

  FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));

  if (fNewMap) {
    fNewMap = FALSE;

    giMiniMap = AddVideoSurface(88, 44, PIXEL_DEPTH);

    // USING BRET's STUFF FOR LOOPING FILES/CREATING LIST, hence
    // AddToFDlgList.....
    try {
      std::vector<std::string> files = FindFilesInDir(MAPSDIR, ".dat", true, true, true);
      for (const std::string &file : files) {
        FileList = AddToFDlgList(FileList, file.c_str());
        ++sFiles;
      }
    } catch (...) { /* XXX ignore */
    }

    FListNode = FileList;

    // Allocate 24 bit Surface
    p24BitValues = MALLOCN(SGPPaletteEntry, MINIMAP_X_SIZE * MINIMAP_Y_SIZE);

    // Allocate 8-bit surface
    gi8BitMiniMap = AddVideoSurface(88, 44, 8);
  }

  // OK, we are here, now loop through files
  if (sCurFile == sFiles || FListNode == NULL) {
    requestGameExit();
    return (MAPUTILITY_SCREEN);
  }

  char zFilename[260];
  strcpy(zFilename, FListNode->filename);

  // OK, load maps and do overhead shrinkage of them...
  try {
    LoadWorld(zFilename);
  } catch (...) {
    return ERROR_SCREEN;
  }

  // Render small map
  InitNewOverheadDB(giCurrentTilesetID);

  gfOverheadMapDirty = TRUE;

  RenderOverheadMap(0, WORLD_COLS / 2, 0, 0, SCREEN_WIDTH, 320, TRUE);

  TrashOverheadMap();

  // OK, NOW PROCESS OVERHEAD MAP ( SHOUIDL BE ON THE FRAMEBUFFER )
  gdXStep = SCREEN_WIDTH / 88.f;
  gdYStep = 320 / 44.f;
  dStartX = dStartY = 0;

  // Adjust if we are using a restricted map...
  if (gMapInformation.ubRestrictedScrollID != 0) {
    CalculateRestrictedMapCoords(NORTH, &sX1, &sY1, &sX2, &sTop, SCREEN_WIDTH, 320);
    CalculateRestrictedMapCoords(SOUTH, &sX1, &sBottom, &sX2, &sY2, SCREEN_WIDTH, 320);
    CalculateRestrictedMapCoords(WEST, &sX1, &sY1, &sLeft, &sY2, SCREEN_WIDTH, 320);
    CalculateRestrictedMapCoords(EAST, &sRight, &sY1, &sX2, &sY2, SCREEN_WIDTH, 320);

    gdXStep = (float)(sRight - sLeft) / (float)88;
    gdYStep = (float)(sBottom - sTop) / (float)44;

    dStartX = sLeft;
    dStartY = sTop;
  }

  // LOCK BUFFERS

  dX = dStartX;
  dY = dStartY;

  {
    SGPVSurface::Lock lsrc(FRAME_BUFFER);
    SGPVSurface::Lock ldst(giMiniMap);
    uint16_t *const pSrcBuf = lsrc.Buffer<uint16_t>();
    uint32_t const uiSrcPitchBYTES = lsrc.Pitch();
    uint16_t *const pDestBuf = ldst.Buffer<uint16_t>();
    uint32_t const uiDestPitchBYTES = ldst.Pitch();

    for (iX = 0; iX < 88; iX++) {
      dY = dStartY;

      for (iY = 0; iY < 44; iY++) {
        // OK, AVERAGE PIXELS
        iSubX1 = (int32_t)dX - WINDOW_SIZE;

        iSubX2 = (int32_t)dX + WINDOW_SIZE;

        iSubY1 = (int32_t)dY - WINDOW_SIZE;

        iSubY2 = (int32_t)dY + WINDOW_SIZE;

        iCount = 0;
        bR = bG = bB = 0;

        for (iWindowX = iSubX1; iWindowX < iSubX2; iWindowX++) {
          for (iWindowY = iSubY1; iWindowY < iSubY2; iWindowY++) {
            if (0 <= iWindowX && iWindowX < SCREEN_WIDTH && 0 <= iWindowY && iWindowY < 320) {
              s16BPPSrc = pSrcBuf[(iWindowY * (uiSrcPitchBYTES / 2)) + iWindowX];

              uiRGBColor = GetRGBColor(s16BPPSrc);

              bR += SGPGetRValue(uiRGBColor);
              bG += SGPGetGValue(uiRGBColor);
              bB += SGPGetBValue(uiRGBColor);

              // Average!
              iCount++;
            }
          }
        }

        if (iCount > 0) {
          bAvR = bR / (uint8_t)iCount;
          bAvG = bG / (uint8_t)iCount;
          bAvB = bB / (uint8_t)iCount;

          sDest16BPPColor = Get16BPPColor(FROMRGB(bAvR, bAvG, bAvB));
        }

        // Write into dest!
        pDestBuf[(iY * (uiDestPitchBYTES / 2)) + iX] = sDest16BPPColor;

        SGPPaletteEntry *const dst = &p24BitValues[iY * (uiDestPitchBYTES / 2) + iX];
        dst->r = bAvR;
        dst->g = bAvG;
        dst->b = bAvB;

        // Increment
        dY += gdYStep;
      }

      // Increment
      dX += gdXStep;
    }
  }

  // RENDER!
  BltVideoSurface(FRAME_BUFFER, giMiniMap, 20, 360, NULL);

  char zFilename2[260];
  // QUantize!
  {
    SGPVSurface::Lock lsrc(gi8BitMiniMap);
    uint8_t *const pDataPtr = lsrc.Buffer<uint8_t>();
    {
      SGPVSurface::Lock ldst(FRAME_BUFFER);
      uint16_t *const pDestBuf = ldst.Buffer<uint16_t>();
      uint32_t const uiDestPitchBYTES = ldst.Pitch();
      QuantizeImage(pDataPtr, p24BitValues, MINIMAP_X_SIZE, MINIMAP_Y_SIZE, pPalette);
      gi8BitMiniMap->SetPalette(pPalette);
      // Blit!
      Blt8BPPDataTo16BPPBuffer(pDestBuf, uiDestPitchBYTES, gi8BitMiniMap, pDataPtr, 300, 360);

      // Write palette!
      {
        int32_t cnt;
        int32_t sX = 0, sY = 420;
        uint16_t usLineColor;

        SetClippingRegionAndImageWidth(uiDestPitchBYTES, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        for (cnt = 0; cnt < 256; cnt++) {
          usLineColor = Get16BPPColor(FROMRGB(pPalette[cnt].r, pPalette[cnt].g, pPalette[cnt].b));
          RectangleDraw(TRUE, sX, sY, sX, sY + 10, usLineColor, pDestBuf);
          sX++;
          RectangleDraw(TRUE, sX, sY, sX, sY + 10, usLineColor, pDestBuf);
          sX++;
        }
      }
    }

    // Remove extension
    for (cnt = (int32_t)strlen(zFilename) - 1; cnt >= 0; cnt--) {
      if (zFilename[cnt] == '.') {
        zFilename[cnt] = '\0';
      }
    }

    sprintf(zFilename2, RADARMAPSDIR "/%s.sti", zFilename);
    WriteSTIFile(pDataPtr, pPalette, MINIMAP_X_SIZE, MINIMAP_Y_SIZE, zFilename2,
                 CONVERT_ETRLE_COMPRESS, 0);
  }

  SetFontAttributes(TINYFONT1, FONT_MCOLOR_DKGRAY);
  mprintf(10, 340, L"Writing radar image %hs", zFilename2);
  mprintf(10, 350, L"Using tileset %ls", gTilesets[giCurrentTilesetID].zName);

  InvalidateScreen();

  while (DequeueEvent(&InputEvent)) {
    if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_ESCAPE) {  // Exit the program
      requestGameExit();
    }
  }

  // Set next
  FListNode = FListNode->pNext;
  sCurFile++;

  return (MAPUTILITY_SCREEN);
}
