#ifndef LOADSAVEOBJECTTYPE_H
#define LOADSAVEOBJECTTYPE_H

#include "Tactical/ItemTypes.h"

const uint8_t *ExtractObject(const uint8_t *Src, OBJECTTYPE *o);

uint8_t *InjectObject(uint8_t *Dst, const OBJECTTYPE *o);

#endif
