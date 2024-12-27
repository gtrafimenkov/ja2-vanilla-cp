// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _JA2_LOAD_SCREEN_
#define _JA2_LOAD_SCREEN_

#include "SGP/Types.h"
#include "ScreenIDs.h"

struct FDLG_LIST {
  char filename[260];
  FDLG_LIST *pNext;
  FDLG_LIST *pPrev;
};

FDLG_LIST *AddToFDlgList(FDLG_LIST *, char const *filename);

BOOLEAN ExternalLoadMap(const wchar_t *szFilename);
BOOLEAN ExternalSaveMap(const wchar_t *szFilename);

void SetErrorCatchString(wchar_t const *fmt, ...);

ScreenID LoadSaveScreenHandle();

#endif
