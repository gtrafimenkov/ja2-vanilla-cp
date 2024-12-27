// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __EDITORTERRAIN_H
#define __EDITORTERRAIN_H

#include "SGP/Types.h"

#define TERRAIN_TILES_NODRAW 0
#define TERRAIN_TILES_FOREGROUND 1
#define TERRAIN_TILES_BACKGROUND 2
// Andrew, could you figure out what the hell mode this is???
// It somehow links terrain tiles with lights and buildings.
#define TERRAIN_TILES_BRETS_STRANGEMODE 3

// Soon to be added to an editor struct
extern uint16_t usTotalWeight;
extern BOOLEAN fPrevShowTerrainTileButtons;
extern BOOLEAN fUseTerrainWeights;
extern int32_t TerrainTileSelected, TerrainForegroundTile, TerrainBackgroundTile;
extern int32_t TerrainTileDrawMode;

void EntryInitEditorTerrainInfo();
void RenderTerrainTileButtons();

void ResetTerrainTileWeights();
void ShowTerrainTileButtons();
void HideTerrainTileButtons();

void ChooseWeightedTerrainTile();

void TerrainFill(uint32_t iMapIndex);

#endif
