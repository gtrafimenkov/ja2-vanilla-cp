//------------------------------------------------------------------------------
// Cinematics Module
//
//
//	Stolen from Nemesis by Derek Beland.
//	Originally by Derek Beland and Bret Rowden.
//
//------------------------------------------------------------------------------

#include "Utils/Cinematics.h"

#include <string.h>

#include "Intro.h"
#include "Local.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/HImage.h"
#include "SGP/SmackStub.h"
#include "SGP/SoundMan.h"
#include "SGP/Types.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"

struct SMKFLIC {
  HWFILE hFileHandle;
  Smack *SmackerObject;
  char SmackerStatus;
  SDL_Surface *SmackBuffer;
  uint32_t uiFlags;
  uint32_t uiLeft;
  uint32_t uiTop;
};

// SMKFLIC uiFlags
#define SMK_FLIC_OPEN 0x00000001       // Flic is open
#define SMK_FLIC_PLAYING 0x00000002    // Flic is playing
#define SMK_FLIC_AUTOCLOSE 0x00000008  // Close when done

static SMKFLIC SmkList[4];
static uint32_t guiSmackPixelFormat = SMACKBUFFER565;

BOOLEAN SmkPollFlics() {
  BOOLEAN fFlicStatus = FALSE;
  FOR_EACH(SMKFLIC, i, SmkList) {
    if (!(i->uiFlags & SMK_FLIC_PLAYING)) continue;
    fFlicStatus = TRUE;

    Smack *const smkobj = i->SmackerObject;

    if (SmackWait(smkobj)) continue;

    // if (SmackSkipFrames(smkobj)) continue;

    {
      SGPVSurface::Lock l(FRAME_BUFFER);
      SmackToBuffer(smkobj, i->uiLeft, i->uiTop, l.Pitch(), smkobj->Height, smkobj->Width,
                    l.Buffer<uint16_t>(), guiSmackPixelFormat);
      SmackDoFrame(smkobj);
    }

    // Check to see if the flic is done the last frame
    // printf ("smk->FrameNum %u\n", smk->FrameNum);
    // if (smk->FrameNum == smk->Frames - 1)
    if (i->SmackerStatus == SMK_LAST) {
      if (i->uiFlags & SMK_FLIC_AUTOCLOSE) SmkCloseFlic(i);
    } else {
      i->SmackerStatus = SmackNextFrame(smkobj);
    }
  }

  return fFlicStatus;
}

void SmkInitialize() {
  // Wipe the flic list clean
  memset(SmkList, 0, sizeof(SmkList));

  // Use MMX acceleration, if available
  SmackUseMMX(1);
}

void SmkShutdown() {
  // Close and deallocate any open flics
  FOR_EACH(SMKFLIC, i, SmkList) {
    if (i->uiFlags & SMK_FLIC_OPEN) SmkCloseFlic(i);
  }
}

static SMKFLIC *SmkOpenFlic(const char *filename);

SMKFLIC *SmkPlayFlic(const char *const filename, const uint32_t left, const uint32_t top,
                     const BOOLEAN auto_close) {
  SMKFLIC *const sf = SmkOpenFlic(filename);
  if (sf == NULL) return NULL;

  // Set the blitting position on the screen
  sf->uiLeft = left;
  sf->uiTop = top;

  // We're now playing, flag the flic for the poller to update
  sf->uiFlags |= SMK_FLIC_PLAYING;
  if (auto_close) sf->uiFlags |= SMK_FLIC_AUTOCLOSE;

  return sf;
}

static SMKFLIC *SmkGetFreeFlic();
static void SmkSetupVideo();

static SMKFLIC *SmkOpenFlic(const char *const filename) try {
  SMKFLIC *const sf = SmkGetFreeFlic();
  if (!sf) {
    FastDebugMsg("SMK ERROR: Out of flic slots, cannot open another");
    return NULL;
  }

  AutoSGPFile file(FileMan::openForReadingSmart(filename, true));

  // FILE* const f = GetRealFileHandleFromFileManFileHandle(file);

  // printf("File Size: %u\n", FileGetSize(file));
  // Allocate a Smacker buffer for video decompression
  /*
  sf->SmackBuffer = SmackBufferOpen(SMACKAUTOBLIT, SCREEN_WIDTH, SCREEN_HEIGHT,
  0, 0); if (sf->SmackBuffer == NULL)
  {
          FastDebugMsg("SMK ERROR: Can't allocate a Smacker decompression
  buffer"); return NULL;
  }
  */
  sf->SmackerObject = SmackOpen(file, SMACKFILEHANDLE | SMACKTRACKS, SMACKAUTOEXTRA);
  if (!sf->SmackerObject) {
    FastDebugMsg("SMK ERROR: Smacker won't open the SMK file");
    return NULL;
  }

  // Make sure we have a video surface
  SmkSetupVideo();

  sf->hFileHandle = file.Release();
  sf->uiFlags |= SMK_FLIC_OPEN;
  return sf;
} catch (...) {
  return 0;
}

void SmkCloseFlic(SMKFLIC *const sf) {
  SmackClose(sf->SmackerObject);
  // FileClose(sf->hFileHandle);
  // reenable for blitting SmackBufferClose(sf->SmackBuffer);
  memset(sf, 0, sizeof(*sf));
}

static SMKFLIC *SmkGetFreeFlic() {
  FOR_EACH(SMKFLIC, i, SmkList) {
    if (!(i->uiFlags & SMK_FLIC_OPEN)) return i;
  }
  return NULL;
}

static void SmkSetupVideo() {
  uint32_t red;
  uint32_t green;
  uint32_t blue;
  GetPrimaryRGBDistributionMasks(&red, &green, &blue);
  if (red == 0xf800 && green == 0x07e0 && blue == 0x001f) {
    guiSmackPixelFormat = SMACKBUFFER565;
  } else {
    guiSmackPixelFormat = SMACKBUFFER555;
  }
}
