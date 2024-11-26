#include "Timer.h"

#include "SDL_timer.h"

uint32_t GetClock() { return SDL_GetTicks(); }
