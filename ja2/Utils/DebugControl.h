// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __DEBUG_CONTROL_
#define __DEBUG_CONTROL_

#include "SGP/Types.h"

void LiveMessage(const char *strMessage);

#ifdef _ANIMSUBSYSTEM_DEBUG

#define AnimDebugMsg(c) AnimDbgMessage((c))

extern void AnimDbgMessage(char *Str);

#else

#define AnimDebugMsg(c) (void)0

#endif

#ifdef _PHYSICSSUBSYSTEM_DEBUG

#define PhysicsDebugMsg(c) PhysicsDbgMessage((c))

extern void PhysicsDbgMessage(char *Str);

#else

#define PhysicsDebugMsg(c) (void)0

#endif

#ifdef _AISUBSYSTEM_DEBUG

#define AiDebugMsg(c) AiDbgMessage((c))

extern void AiDbgMessage(char *Str);

#else

#define AiDebugMsg(c) (void)0

#endif

#endif
