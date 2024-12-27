// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __INTERFACE_UTILS_H
#define __INTERFACE_UTILS_H

#include "JA2Types.h"
#include "Tactical/ItemTypes.h"

#define DRAW_ITEM_STATUS_ATTACHMENT1 200
#define DRAW_ITEM_STATUS_ATTACHMENT2 201
#define DRAW_ITEM_STATUS_ATTACHMENT3 202
#define DRAW_ITEM_STATUS_ATTACHMENT4 203

void DrawSoldierUIBars(SOLDIERTYPE const &, int16_t sXPos, int16_t sYPos, BOOLEAN fErase,
                       SGPVSurface *buffer);

void DrawItemUIBarEx(OBJECTTYPE const &, uint8_t ubStatus, int16_t sXPos, int16_t sYPos,
                     int16_t sHeight, int16_t sColor1, int16_t sColor2, SGPVSurface *buffer);

void RenderSoldierFace(SOLDIERTYPE const &, int16_t sFaceX, int16_t sFaceY);

// load portraits for cars
void LoadCarPortraitValues();

// get rid of the loaded portraits for cars
void UnLoadCarPortraits();

void LoadInterfaceUtilsGraphics();
void DeleteInterfaceUtilsGraphics();

#endif
