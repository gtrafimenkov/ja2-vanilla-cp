// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __EDITORMERCS_H
#define __EDITORMERCS_H

#include "JA2Types.h"

// Merc editing modes.  These are used to determine which buttons to show and
// hide.
enum {
  MERC_NOMODE,  // used for shutting down mercs tab, to extract any changed
                // information

  MERC_GETITEMMODE,  // when selecting a specific piece of inventory from
                     // inventorymode

  MERC_TEAMMODE,   // selecting a team with no merc selected.
  MERC_BASICMODE,  // basic placement mode

  MERC_GENERALMODE,     // detailed placement mode for general information and NPC
                        // info
  MERC_ATTRIBUTEMODE,   // detailed placement mode for specifying attributes
  MERC_INVENTORYMODE,   // detailed placement mode for specifying inventory
  MERC_APPEARANCEMODE,  // detailed placement mode for specifying appearance
  MERC_PROFILEMODE,     // specifying a valid profile index will generate the merc
                        // automatically.
  MERC_SCHEDULEMODE,    // specifying a schedule for that particular individual
};

extern uint8_t gubCurrMercMode;

#define EDIT_NUM_COLORS 4
#define EDIT_COLOR_HEAD 0
#define EDIT_COLOR_PANTS 1
#define EDIT_COLOR_SKIN 2
#define EDIT_COLOR_VEST 3
#define EDIT_MERC_NONE 0
#define EDIT_MERC_NEXT_COLOR 13

extern SOLDIERTYPE *g_selected_merc;
extern int16_t gsSelectedMercGridNo;

enum _ForUseWithIndicateSelectedMerc {
  SELECT_NEXT_CREATURE = -7,
  SELECT_NEXT_REBEL = -6,
  SELECT_NEXT_CIV = -5,
  SELECT_NEXT_ENEMY = -4,
  SELECT_NEXT_TEAMMATE = -3,
  SELECT_NEXT_MERC = -2,
  SELECT_NO_MERC = -1
  // >= 0 select merc with matching ID
};

void IndicateSelectedMerc(int16_t sID);

void GameInitEditorMercsInfo();
void GameShutdownEditorMercsInfo();
void EntryInitEditorMercsInfo();
void UpdateMercsInfo();

void ProcessMercEditing();
void AddMercToWorld(int32_t iMapIndex);
void HandleRightClickOnMerc(int32_t iMapIndex);
void SetMercEditingMode(uint8_t ubNewMode);

void ResetAllMercPositions();

void EraseMercWaypoint();
void AddMercWaypoint(uint32_t iMapIndex);

void SetEnemyColorCode(uint8_t ubColorCode);

void SpecifyEntryPoint(uint32_t iMapIndex);

// Modify stats of current soldiers
void SetMercOrders(int8_t bOrders);
void SetMercAttitude(int8_t bAttitude);
void SetMercDirection(int8_t bDirection);
void SetMercRelativeEquipment(int8_t bLevel);
void SetMercRelativeAttributes(int8_t bLevel);

void DeleteSelectedMerc();

void ExtractCurrentMercModeInfo(BOOLEAN fKillTextInputMode);

void HandleMercInventoryPanel(int16_t sX, int16_t sY, int8_t bEvent);

extern uint16_t gusMercsNewItemIndex;
extern BOOLEAN gfRenderMercInfo;

void ChangeCivGroup(uint8_t ubNewCivGroup);

#define MERCINV_LGSLOT_WIDTH 48
#define MERCINV_SMSLOT_WIDTH 24
#define MERCINV_SLOT_HEIGHT 18

extern BOOLEAN gfRoofPlacement;

extern void SetEnemyDroppableStatus(uint32_t uiSlot, BOOLEAN fDroppable);

void RenderMercStrings();

extern BOOLEAN gfShowPlayers;
extern BOOLEAN gfShowEnemies;
extern BOOLEAN gfShowCreatures;
extern BOOLEAN gfShowRebels;
extern BOOLEAN gfShowCivilians;
void SetMercTeamVisibility(int8_t bTeam, BOOLEAN fVisible);

extern uint8_t gubCurrentScheduleActionIndex;
extern BOOLEAN gfSingleAction;
extern BOOLEAN gfUseScheduleData2;

void UpdateScheduleAction(uint8_t ubNewAction);
void FindScheduleGridNo(uint8_t ubScheduleData);
void ClearCurrentSchedule();
void CancelCurrentScheduleAction();
void RegisterCurrentScheduleAction(int32_t iMapIndex);
void StartScheduleAction();

void InitDetailedPlacementForMerc();
void KillDetailedPlacementForMerc();

void CopyMercPlacement(int32_t iMapIndex);
void PasteMercPlacement(int32_t iMapIndex);

void ExtractAndUpdateMercSchedule();

void DeleteSelectedMercsItem();

#endif
