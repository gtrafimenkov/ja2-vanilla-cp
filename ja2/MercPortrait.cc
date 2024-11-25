// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "MercPortrait.h"

#include <stdio.h>

#include "Directories.h"
#include "Macro.h"
#include "SGP/VObject.h"
#include "Tactical/SoldierProfileType.h"

static SGPVObject *LoadPortrait(MERCPROFILESTRUCT const &p, char const *const subdir) {
  SGPFILENAME filename;
  snprintf(filename, lengthof(filename), FACESDIR "/%s%02d.sti", subdir, p.ubFaceIndex);
  return AddVideoObjectFromFile(filename);
}

SGPVObject *Load33Portrait(MERCPROFILESTRUCT const &p) { return LoadPortrait(p, "33face/"); }
SGPVObject *Load65Portrait(MERCPROFILESTRUCT const &p) { return LoadPortrait(p, "65face/"); }
SGPVObject *LoadBigPortrait(MERCPROFILESTRUCT const &p) { return LoadPortrait(p, "bigfaces/"); }
SGPVObject *LoadSmallPortrait(MERCPROFILESTRUCT const &p) { return LoadPortrait(p, ""); }
