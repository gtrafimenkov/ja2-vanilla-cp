#ifndef __IMP_MAINPAGE_H
#define __IMP_MAINPAGE_H
#include "SGP/Types.h"

void RenderIMPMainPage();
void ExitIMPMainPage();
void EnterIMPMainPage();
void HandleIMPMainPage();

extern int32_t iCurrentProfileMode;

SGPVObject *LoadIMPPortait();

#endif
