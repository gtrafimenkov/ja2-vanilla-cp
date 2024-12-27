// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEOBJECTTYPE_H
#define LOADSAVEOBJECTTYPE_H

#include "Tactical/ItemTypes.h"

const uint8_t *ExtractObject(const uint8_t *Src, OBJECTTYPE *o);

uint8_t *InjectObject(uint8_t *Dst, const OBJECTTYPE *o);

#endif
