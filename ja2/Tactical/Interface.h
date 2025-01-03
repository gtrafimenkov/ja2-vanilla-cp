// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "SGP/MouseSystem.h"
#include "Tactical/HandleUI.h"

#define MAX_UICOMPOSITES 4

#define INTERFACE_START_Y (SCREEN_HEIGHT - 120)
#define INV_INTERFACE_START_Y (SCREEN_HEIGHT - 140)

#define INTERFACE_START_X 0

// FLASH PORTRAIT CODES
#define FLASH_PORTRAIT_STOP 0
#define FLASH_PORTRAIT_START 1
#define FLASH_PORTRAIT_WAITING 2
#define FLASH_PORTRAIT_DELAY 150

// FLASH PORTRAIT PALETTE IDS
#define FLASH_PORTRAIT_NOSHADE 0
#define FLASH_PORTRAIT_STARTSHADE 1
#define FLASH_PORTRAIT_ENDSHADE 2
#define FLASH_PORTRAIT_DARKSHADE 3
#define FLASH_PORTRAIT_GRAYSHADE 4
#define FLASH_PORTRAIT_LITESHADE 5

// GLOBAL DEFINES FOR SOME UI FLAGS
#define ARROWS_HIDE_UP 0x00000002
#define ARROWS_HIDE_DOWN 0x00000004
#define ARROWS_SHOW_UP_BESIDE 0x00000008
#define ARROWS_SHOW_DOWN_BESIDE 0x00000020
#define ARROWS_SHOW_UP_ABOVE_Y 0x00000040
#define ARROWS_SHOW_DOWN_BELOW_Y 0x00000080
#define ARROWS_SHOW_DOWN_BELOW_G 0x00000200
#define ARROWS_SHOW_DOWN_BELOW_YG 0x00000400
#define ARROWS_SHOW_DOWN_BELOW_GG 0x00000800
#define ARROWS_SHOW_UP_ABOVE_G 0x00002000
#define ARROWS_SHOW_UP_ABOVE_YG 0x00004000
#define ARROWS_SHOW_UP_ABOVE_GG 0x00008000
#define ARROWS_SHOW_UP_ABOVE_YY 0x00020000
#define ARROWS_SHOW_DOWN_BELOW_YY 0x00040000
#define ARROWS_SHOW_UP_ABOVE_CLIMB 0x00080000
#define ARROWS_SHOW_UP_ABOVE_CLIMB2 0x00400000
#define ARROWS_SHOW_UP_ABOVE_CLIMB3 0x00800000
#define ARROWS_SHOW_DOWN_CLIMB 0x02000000

#define ROOF_LEVEL_HEIGHT 50

// Interface level enums
enum { I_GROUND_LEVEL, I_ROOF_LEVEL, I_NUMLEVELS };

enum InterfacePanelKind { SM_PANEL, TEAM_PANEL, NUM_UI_PANELS };

extern BOOLEAN gfUIStanceDifferent;
extern InterfacePanelKind gsCurInterfacePanel;

extern SGPVObject *guiDEAD;
extern SGPVObject *guiHATCH;
extern SGPVObject *guiRADIO;

extern MOUSE_REGION gViewportRegion;
extern MOUSE_REGION gRadarRegion;

#define MOVEMENT_MENU_LOOK 1
#define MOVEMENT_MENU_ACTIONC 2
#define MOVEMENT_MENU_HAND 3
#define MOVEMENT_MENU_TALK 4
#define MOVEMENT_MENU_RUN 5
#define MOVEMENT_MENU_WALK 6
#define MOVEMENT_MENU_SWAT 7
#define MOVEMENT_MENU_PRONE 8

enum DirtyLevel { DIRTYLEVEL0 = 0, DIRTYLEVEL1 = 1, DIRTYLEVEL2 = 2 };

void InitializeTacticalInterface();
extern DirtyLevel fInterfacePanelDirty;
extern BOOLEAN gfPausedTacticalRenderFlags;
extern DirtyLevel gfPausedTacticalRenderInterfaceFlags;
extern int16_t gsInterfaceLevel;
extern BOOLEAN gfInMovementMenu;

void PopupMovementMenu(UI_EVENT *pUIEvent);
void PopDownMovementMenu();
void RenderMovementMenu();
void CancelMovementMenu();

void PopDownOpenDoorMenu();
void RenderOpenDoorMenu();
void InitDoorOpenMenu(SOLDIERTYPE *pSoldier, BOOLEAN fClosingDoor);
BOOLEAN HandleOpenDoorMenu();
void CancelOpenDoorMenu();

void HandleInterfaceBackgrounds();

void DrawSelectedUIAboveGuy(SOLDIERTYPE &);

void CreateCurrentTacticalPanelButtons();
void RemoveCurrentTacticalPanelButtons();
void SetCurrentTacticalPanelCurrentMerc(SOLDIERTYPE *s);
void SetCurrentInterfacePanel(InterfacePanelKind);
BOOLEAN IsMercPortraitVisible(const SOLDIERTYPE *s);

void InitializeCurrentPanel();
void ShutdownCurrentPanel();

void ClearInterface();
void RestoreInterface();

void RenderArrows();
void EraseRenderArrows();

#define EndDeadlockMsg() ((void)0)

void DirtyMercPanelInterface(SOLDIERTYPE const *, DirtyLevel);

void EndUIMessage();
void BeginUIMessage(BOOLEAN fUseSkullIcon, const wchar_t *text);

// map screen version, for centering over the map area
void BeginMapUIMessage(int16_t delta_y, const wchar_t *text);

extern VIDEO_OVERLAY *g_ui_message_overlay;
extern uint32_t guiUIMessageTime;

enum MESSAGE_TYPES {
  NO_MESSAGE,
  COMPUTER_TURN_MESSAGE,
  COMPUTER_INTERRUPT_MESSAGE,
  PLAYER_INTERRUPT_MESSAGE,
  MILITIA_INTERRUPT_MESSAGE,
  AIR_RAID_TURN_MESSAGE,
  PLAYER_TURN_MESSAGE
};

void HandleTopMessages();
void AddTopMessage(MESSAGE_TYPES ubType);
void EndTopMessage();

void InitEnemyUIBar(uint8_t ubNumEnemies, uint8_t ubDoneEnemies);

const wchar_t *GetSoldierHealthString(const SOLDIERTYPE *s);

void ResetPhysicsTrajectoryUI();
void SetupPhysicsTrajectoryUI();
void EndPhysicsTrajectoryUI();
void BeginPhysicsTrajectoryUI(int16_t sGridNo, int8_t bLevel, BOOLEAN fBadCTGT);

void InitPlayerUIBar(BOOLEAN fInterrupt);

void ToggleTacticalPanels();

void DirtyTopMessage();

void BeginMultiPurposeLocator(int16_t sGridNo, int8_t bLevel);
void HandleMultiPurposeLocator();
void RenderTopmostMultiPurposeLocator();

void GetSoldierAboveGuyPositions(const SOLDIERTYPE *s, int16_t *psX, int16_t *psY, BOOLEAN fRadio);

void UpdateEnemyUIBar();

extern BOOLEAN gfInOpenDoorMenu;
extern uint32_t guiUIMessageTimeDelay;
extern BOOLEAN gfTopMessageDirty;

#endif
