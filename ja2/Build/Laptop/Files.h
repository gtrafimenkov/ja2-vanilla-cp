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
