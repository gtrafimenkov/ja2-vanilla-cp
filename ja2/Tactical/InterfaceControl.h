// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __INTERFACE_CONTROL_H
#define __INTERFACE_CONTROL_H

#include "JA2Types.h"

void SetUpInterface();
void ResetInterface();
void RenderTopmostTacticalInterface();
void RenderTacticalInterface();

void RenderTacticalInterfaceWhileScrolling();

void EraseInterfaceMenus(BOOLEAN fIgnoreUIUnLock);

void ResetInterfaceAndUI();

bool AreWeInAUIMenu();

void HandleTacticalPanelSwitch();

bool InterfaceOKForMeanwhilePopup();

extern BOOLEAN gfRerenderInterfaceFromHelpText;

/* If given a null pointer, show the team panel. Otherwise show the given merc
 * in the single merc panel. */
void SetNewPanel(SOLDIERTYPE *);

#endif
