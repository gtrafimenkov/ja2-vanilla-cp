#ifndef __INTERACTIVE_TILES_H
#define __INTERACTIVE_TILES_H

#include "JA2Types.h"
#include "Tactical/InterfaceCursors.h"
#include "TileEngine/WorldDef.h"

#define INTTILE_DOOR_OPENSPEED 70

void StartInteractiveObject(GridNo, STRUCTURE const &, SOLDIERTYPE &, uint8_t direction);
BOOLEAN StartInteractiveObjectFromMouse(SOLDIERTYPE *pSoldier, uint8_t ubDirection);
UICursorID GetInteractiveTileCursor(UICursorID old_cursor, BOOLEAN fConfirm);
bool SoldierHandleInteractiveObject(SOLDIERTYPE &);

void HandleStructChangeFromGridNo(SOLDIERTYPE *, GridNo);

void BeginCurInteractiveTileCheck();
void EndCurInteractiveTileCheck();
void LogMouseOverInteractiveTile(int16_t sGridNo);
BOOLEAN ShouldCheckForMouseDetections();

void CycleIntTileFindStack(uint16_t usMapPos);
void SetActionModeDoorCursorText();

LEVELNODE *GetCurInteractiveTile();
LEVELNODE *GetCurInteractiveTileGridNo(int16_t *psGridNo);
LEVELNODE *GetCurInteractiveTileGridNoAndStructure(int16_t *psGridNo, STRUCTURE **ppStructure);
LEVELNODE *ConditionalGetCurInteractiveTileGridNoAndStructure(int16_t *psGridNo,
                                                              STRUCTURE **ppStructure,
                                                              BOOLEAN fRejectOnTopItems);

BOOLEAN CheckVideoObjectScreenCoordinateInData(HVOBJECT hSrcVObject, uint16_t usIndex,
                                               int32_t iTestX, int32_t iTestY);

#endif
