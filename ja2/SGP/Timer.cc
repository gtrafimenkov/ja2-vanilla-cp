// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Timer.h"

#include "SDL_timer.h"

uint32_t GetClock() { return SDL_GetTicks(); }
