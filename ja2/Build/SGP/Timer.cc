#include "Timer.h"

#include "SDL_timer.h"

UINT32 GetClock() { return SDL_GetTicks(); }
