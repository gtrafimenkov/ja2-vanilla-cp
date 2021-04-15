#ifndef __DEBUG_CONTROL_
#define __DEBUG_CONTROL_

#include "SGP/Types.h"

void LiveMessage(const char *strMessage);

#ifdef _ANIMSUBSYSTEM_DEBUG

#define AnimDebugMsg(c) AnimDbgMessage((c))

extern void AnimDbgMessage(CHAR8 *Str);

#else

#define AnimDebugMsg(c) (void)0

#endif

#ifdef _PHYSICSSUBSYSTEM_DEBUG

#define PhysicsDebugMsg(c) PhysicsDbgMessage((c))

extern void PhysicsDbgMessage(CHAR8 *Str);

#else

#define PhysicsDebugMsg(c) (void)0

#endif

#ifdef _AISUBSYSTEM_DEBUG

#define AiDebugMsg(c) AiDbgMessage((c))

extern void AiDbgMessage(CHAR8 *Str);

#else

#define AiDebugMsg(c) (void)0

#endif

#endif
