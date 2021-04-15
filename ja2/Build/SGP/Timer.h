#ifndef TIMER_H
#define TIMER_H

#include <SDL.h>

#include "SGP/Types.h"

static inline UINT32 GetClock(void) { return SDL_GetTicks(); }

#endif
