#ifndef __EDITOR_UNDO_H
#define __EDITOR_UNDO_H

#include "SGP/Types.h"

BOOLEAN AddToUndoList(int32_t iMapIndex);
void AddLightToUndoList(int32_t iMapIndex, int32_t iLightRadius);

void RemoveAllFromUndoList();
BOOLEAN ExecuteUndoList();

void EnableUndo();
void DisableUndo();

void DetermineUndoState();

// Undo command flags
#define MAX_UNDO_COMMAND_LENGTH 10

#endif
