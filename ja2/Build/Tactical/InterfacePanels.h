// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __INTERFACE_PANELS
#define __INTERFACE_PANELS

#include "JA2Types.h"
#include "SGP/ButtonSystem.h"
#include "SGP/MouseSystem.h"
#include "Tactical/Interface.h"

enum {
  STANCEUP_BUTTON = 0,
  UPDOWN_BUTTON,
  CLIMB_BUTTON,
  STANCEDOWN_BUTTON,
  HANDCURSOR_BUTTON,
  PREVMERC_BUTTON,
  NEXTMERC_BUTTON,
  OPTIONS_BUTTON,
  BURSTMODE_BUTTON,
  LOOK_BUTTON,
  TALK_BUTTON,
  MUTE_BUTTON,
  SM_DONE_BUTTON,
  SM_MAP_SCREEN_BUTTON,
  NUM_SM_BUTTONS
};

enum { TEAM_DONE_BUTTON = 0, TEAM_MAP_SCREEN_BUTTON, CHANGE_SQUAD_BUTTON, NUM_TEAM_BUTTONS };

#define NEW_ITEM_CYCLE_COUNT 19
#define NEW_ITEM_CYCLES 4
#define NUM_TEAM_SLOTS 6

#define PASSING_ITEM_DISTANCE_OKLIFE 3
#define PASSING_ITEM_DISTANCE_NOTOKLIFE 2

#define SHOW_LOCATOR_NORMAL 1
#define SHOW_LOCATOR_FAST 2

void CreateSMPanelButtons();
void RemoveSMPanelButtons();
void InitializeSMPanel();
void ShutdownSMPanel();
void RenderSMPanel(DirtyLevel *);
void EnableSMPanelButtons(BOOLEAN fEnable, BOOLEAN fFromItemPickup);

void CreateTEAMPanelButtons();
void RemoveTEAMPanelButtons();
void InitializeTEAMPanel();
void ShutdownTEAMPanel();
void RenderTEAMPanel(DirtyLevel);

void SetSMPanelCurrentMerc(SOLDIERTYPE *s);
void SetTEAMPanelCurrentMerc();

void InitTEAMSlots();
SOLDIERTYPE *GetPlayerFromInterfaceTeamSlot(uint8_t ubPanelSlot);
void RemoveAllPlayersFromSlot();
BOOLEAN RemovePlayerFromTeamSlot(const SOLDIERTYPE *s);
void CheckForAndAddMercToTeamPanel(SOLDIERTYPE *s);

void DisableTacticalTeamPanelButtons(BOOLEAN fDisable);
void RenderTownIDString();

void KeyRingItemPanelButtonCallback(MOUSE_REGION *pRegion, int32_t iReason);
void KeyRingSlotInvClickCallback(MOUSE_REGION *pRegion, int32_t iReason);

void ShowRadioLocator(SOLDIERTYPE *s, uint8_t ubLocatorSpeed);
void EndRadioLocator(SOLDIERTYPE *s);

extern MOUSE_REGION gSMPanelRegion;

extern BOOLEAN gfDisableTacticalPanelButtons;

// Used when the shop keeper interface is active
void DisableSMPpanelButtonsWhenInShopKeeperInterface();

void ReEvaluateDisabledINVPanelButtons();

void CheckForDisabledForGiveItem();
void ReevaluateItemHatches(SOLDIERTYPE *s, BOOLEAN fEnable);

void HandlePanelFaceAnimations(SOLDIERTYPE *s);

void GoToMapScreenFromTactical();

void HandleTacticalEffectsOfEquipmentChange(SOLDIERTYPE *s, uint32_t uiInvPos, uint16_t usOldItem,
                                            uint16_t usNewItem);

void FinishAnySkullPanelAnimations();

SOLDIERTYPE *FindNextMercInTeamPanel(SOLDIERTYPE *prev);

void BeginKeyPanelFromKeyShortcut();

void UpdateForContOverPortrait(SOLDIERTYPE *s, BOOLEAN fOn);

void HandleLocateSelectMerc(SOLDIERTYPE *, bool force_select);

BOOLEAN HandleNailsVestFetish(const SOLDIERTYPE *pSoldier, uint32_t uiHandPos,
                              uint16_t usReplaceItem);

extern SOLDIERTYPE *gpSMCurrentMerc;
extern GUIButtonRef iSMPanelButtons[NUM_SM_BUTTONS];
extern GUIButtonRef iTEAMPanelButtons[NUM_TEAM_BUTTONS];
extern GUIButtonRef giSMStealthButton;
extern SOLDIERTYPE *gSelectSMPanelToMerc;
extern MOUSE_REGION gSM_SELMERCMoneyRegion;
extern uint8_t gubHandPos;
extern uint16_t gusOldItemIndex;
extern uint16_t gusNewItemIndex;
extern BOOLEAN gfDeductPoints;
extern BOOLEAN gfSMDisableForItems;

void LoadInterfacePanelGraphics();
void DeleteInterfacePanelGraphics();

#endif
