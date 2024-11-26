#include "Utils/DebugControl.h"

#include <stdio.h>

#include "SGP/Types.h"

#ifdef _ANIMSUBSYSTEM_DEBUG

void AnimDbgMessage(char *strMessage) {
  FILE *OutFile;

  if ((OutFile = fopen("AnimDebug.txt", "a+")) != NULL) {
    fprintf(OutFile, "%s\n", strMessage);
    fclose(OutFile);
  }
}

#endif

#ifdef _PHYSICSSUBSYSTEM_DEBUG

void PhysicsDbgMessage(char *strMessage) {
  FILE *OutFile;

  if ((OutFile = fopen("PhysicsDebug.txt", "a+")) != NULL) {
    fprintf(OutFile, "%s\n", strMessage);
    fclose(OutFile);
  }
}

#endif

#ifdef _AISUBSYSTEM_DEBUG

void AiDbgMessage(char *strMessage) {
  FILE *OutFile;

  if ((OutFile = fopen("AiDebug.txt", "a+")) != NULL) {
    fprintf(OutFile, "%s\n", strMessage);
    fclose(OutFile);
  }
}

#endif

void LiveMessage(const char *strMessage) {
  FILE *OutFile;

  if ((OutFile = fopen("Log.txt", "a+")) != NULL) {
    fprintf(OutFile, "%s\n", strMessage);
    fclose(OutFile);
  }
}
