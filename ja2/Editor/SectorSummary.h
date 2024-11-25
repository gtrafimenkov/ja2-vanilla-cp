// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SECTOR_SUMMARY_H
#define __SECTOR_SUMMARY_H

#include "SGP/Input.h"

void CreateSummaryWindow();
void DestroySummaryWindow();
void RenderSummaryWindow();
void LoadWorldInfo();

void UpdateSectorSummary(const wchar_t *gszFilename, BOOLEAN fUpdate);

void SaveGlobalSummary();

extern BOOLEAN gfGlobalSummaryExists;

extern BOOLEAN gfSummaryWindowActive;

void AutoLoadMap();

BOOLEAN HandleSummaryInput(InputAtom *);

#endif
