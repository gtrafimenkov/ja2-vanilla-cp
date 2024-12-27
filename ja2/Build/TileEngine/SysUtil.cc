// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/SysUtil.h"

#include "Local.h"
#include "SGP/VSurface.h"

SGPVSurface *guiSAVEBUFFER;
SGPVSurface *guiEXTRABUFFER;

void InitializeGameVideoObjects() {
  guiSAVEBUFFER = AddVideoSurface(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_DEPTH);
  guiEXTRABUFFER = AddVideoSurface(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_DEPTH);
}
