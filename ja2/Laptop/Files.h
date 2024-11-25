// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __FILES_H
#define __FILES_H

#include "SGP/Types.h"

#define FILES_DAT_FILE TEMPDIR "/files.dat"

void GameInitFiles();
void EnterFiles();
void ExitFiles();
void HandleFiles();
void RenderFiles();

extern BOOLEAN fEnteredFileViewerFromNewFileIcon;
extern BOOLEAN fNewFilesInFileViewer;

// add all files about terrorists
void AddFilesAboutTerrorists();

#endif
