#include "Timer.h"

#include "SDL_timer.h"

UINT32 GetClock(void) { return SDL_GetTicks(); }
