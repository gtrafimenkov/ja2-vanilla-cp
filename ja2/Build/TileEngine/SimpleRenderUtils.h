#ifndef __SIMPLE_RENDER_UTILS_H
#define __SIMPLE_RENDER_UTILS_H

#include "SGP/Types.h"

void MarkMapIndexDirty(int32_t iMapIndex);
void CenterScreenAtMapIndex(int32_t iMapIndex);
void MarkWorldDirty();

#endif
