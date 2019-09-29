#ifndef TIMER_H
#define TIMER_H

#include "sgp/Types.h"
#include <SDL.h>

static inline UINT32 GetClock(void)
{
  return SDL_GetTicks();
}

#endif
