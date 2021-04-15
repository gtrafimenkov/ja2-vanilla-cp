#ifndef LOADSAVEOBJECTTYPE_H
#define LOADSAVEOBJECTTYPE_H

#include "Tactical/ItemTypes.h"


const BYTE* ExtractObject(const BYTE* Src, OBJECTTYPE* o);

BYTE* InjectObject(BYTE* Dst, const OBJECTTYPE* o);

#endif
