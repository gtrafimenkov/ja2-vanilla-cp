// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/StrategicExitGUI.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "FadeScreen.h"
#include "GameScreen.h"
#include "Local.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/CursorControl.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/MouseSystem.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "Strategic/CreatureSpreading.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/Quests.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMovement.h"
#include "Tactical/Interface.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/Squads.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SysUtil.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/MercTextBox.h"
#include "Utils/PopUpBox.h"
#include "Utils/Text.h"

#include "SDL_keycode.h"

BOOLEAN gfInSectorExitMenu = FALSE;

struct EXIT_DIALOG_STRUCT {
  MOUSE_REGION BackRegion;
  MOUSE_REGION SingleRegion;
  MOUSE_REGION LoadRegion;
  MOUSE_REGION AllRegion;
  GUIButtonRef uiLoadCheckButton;
  GUIButtonRef uiSingleMoveButton;
  GUIButtonRef uiAllMoveButton;
  GUIButtonRef uiOKButton;
  GUIButtonRef uiCancelButton;
  MercPopUpBox *box;
  BUTTON_PICS *iButtonImages;
  uint16_t usWidth;
  uint16_t usHeight;
  int16_t sX;
  int16_t sY;
  int16_t sAdditionalData;
  uint8_t ubFlags;
  uint8_t ubLeaveSectorType;
  uint8_t ubLeaveSectorCode;
  uint8_t ubDirection;
  uint8_t ubNumPeopleOnSquad;
  const SOLDIERTYPE *single_move_will_isolate_epc;  // if not NULL, then that means it is an EPC
  int8_t bHandled;
  BOOLEAN fRender;
  BOOLEAN fGotoSector;
  BOOLEAN fGotoSectorText;
  BOOLEAN fSingleMove;
  BOOLEAN fAllMove;
  BOOLEAN fSingleMoveDisabled;
  BOOLEAN fGotoSectorDisabled;
  BOOLEAN fAllMoveDisabled;
  BOOLEAN fGotoSectorHilighted;
  BOOLEAN fSingleMoveHilighted;
  BOOLEAN fAllMoveHilighted;
  BOOLEAN fMultipleSquadsInSector;
  BOOLEAN fSingleMoveOn;
  BOOLEAN fAllMoveOn;
  BOOLEAN fSelectedMercIsEPC;
  BOOLEAN fSquadHasMultipleEPCs;
  BOOLEAN fUncontrolledRobotInSquad;
};

EXIT_DIALOG_STRUCT gExitDialog;

uint8_t gubExitGUIDirection;
int16_t gsExitGUIAdditionalData;
int16_t gsWarpWorldX;
int16_t gsWarpWorldY;
int8_t gbWarpWorldZ;
int16_t gsWarpGridNo;

static GUIButtonRef MakeButton(const wchar_t *text, int16_t dx, GUI_CALLBACK click) {
  const int16_t text_col = FONT_MCOLOR_WHITE;
  const int16_t shadow_col = DEFAULT_SHADOW;
  return CreateIconAndTextButton(gExitDialog.iButtonImages, text, FONT12ARIAL, text_col, shadow_col,
                                 text_col, shadow_col, gExitDialog.sX + dx, gExitDialog.sY + 78,
                                 MSYS_PRIORITY_HIGHEST, click);
}

static void AllMoveCallback(GUI_BUTTON *btn, int32_t reason);
static void AllRegionCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void AllRegionMoveCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void CancelCallback(GUI_BUTTON *btn, int32_t reason);
static void CheckLoadMapCallback(GUI_BUTTON *btn, int32_t reason);
static void LoadRegionCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void LoadRegionMoveCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void OKCallback(GUI_BUTTON *btn, int32_t reason);
static void SectorExitBackgroundCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void SingleMoveCallback(GUI_BUTTON *btn, int32_t reason);
static void SingleRegionCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void SingleRegionMoveCallback(MOUSE_REGION *pRegion, int32_t iReason);

// KM:  New method is coded for more sophistocated rules.  All the information
// is stored within the gExitDialog struct 		 and calculated upon entry to this
// function instead of passing in multiple arguments and calculating it prior.
static void InternalInitSectorExitMenu(uint8_t const ubDirection, int16_t const sAdditionalData) {
  uint32_t uiTraverseTimeInMinutes;
  SGPRect aRect;
  uint16_t usTextBoxWidth, usTextBoxHeight;
  uint16_t usMapPos = 0;
  int8_t bExitCode = -1;
  BOOLEAN OkExitCode;

  // STEP 1:  Calculate the information for the exit gui
  memset(&gExitDialog, 0, sizeof(EXIT_DIALOG_STRUCT));

  // OK, bring up dialogue... first determine some logic here...
  switch (ubDirection) {
    case EAST:
      bExitCode = EAST_STRATEGIC_MOVE;
      break;
    case WEST:
      bExitCode = WEST_STRATEGIC_MOVE;
      break;
    case NORTH:
      bExitCode = NORTH_STRATEGIC_MOVE;
      break;
    case SOUTH:
      bExitCode = SOUTH_STRATEGIC_MOVE;
      break;
    case DIRECTION_EXITGRID:
      bExitCode = -1;
      usMapPos = sAdditionalData;
      break;
  }

  OkExitCode = OKForSectorExit(bExitCode, usMapPos, &uiTraverseTimeInMinutes);

  if (uiTraverseTimeInMinutes <= 5) {  // if the traverse time is short, then traversal is percieved
                                       // to be instantaneous.
    gExitDialog.fGotoSectorText = TRUE;
  }

  if (OkExitCode == 1) {
    gExitDialog.fAllMoveDisabled = TRUE;
    gExitDialog.fSingleMoveOn = TRUE;
    gExitDialog.fSingleMove = TRUE;
    if (gfRobotWithoutControllerAttemptingTraversal) {
      gfRobotWithoutControllerAttemptingTraversal = FALSE;
      gExitDialog.fUncontrolledRobotInSquad = TRUE;
    }

  } else if (OkExitCode == 2) {
    gExitDialog.fAllMoveOn = TRUE;
    gExitDialog.fAllMove = TRUE;
  }

  if (gTacticalStatus.uiFlags & INCOMBAT) {
    int32_t cnt = 0;
    CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
      if (OkControllableMerc(s)) ++cnt;
    }
    if (cnt != 1) {
      gExitDialog.fGotoSectorDisabled = TRUE;
    }
  }

  // STEP 2:  Setup the exit gui

  EnterModalTactical(TACTICAL_MODAL_WITHMOUSE);
  gfIgnoreScrolling = TRUE;

  aRect.iTop = 0;
  aRect.iLeft = 0;
  aRect.iBottom = INV_INTERFACE_START_Y;
  aRect.iRight = SCREEN_WIDTH;

  if (gExitDialog.fAllMoveOn) {  // either an all-move in non-combat, or the last
                                 // concious guy in combat.
    gExitDialog.fGotoSector = TRUE;
  }

  const SOLDIERTYPE *const sel = GetSelectedMan();
  gExitDialog.ubNumPeopleOnSquad = NumberOfPlayerControllableMercsInSquad(sel->bAssignment);

  // Determine
  CFOR_EACH_IN_TEAM(pSoldier, OUR_TEAM) {
    if (pSoldier == sel) continue;
    if (!pSoldier->fBetweenSectors && pSoldier->sSectorX == gWorldSectorX &&
        pSoldier->sSectorY == gWorldSectorY && pSoldier->bSectorZ == gbWorldSectorZ &&
        pSoldier->bLife >= OKLIFE && pSoldier->bAssignment != sel->bAssignment &&
        pSoldier->bAssignment != ASSIGNMENT_POW && pSoldier->bAssignment != IN_TRANSIT &&
        pSoldier->bAssignment != ASSIGNMENT_DEAD) {  // KM:  We need to determine if there are more
                                                     // than one squad (meaning other concious mercs
                                                     // in a different squad or assignment)
      //		 These conditions were done to the best of my knowledge,
      // so if there are other situations that require modification, 		 then feel
      // free to do so.
      gExitDialog.fMultipleSquadsInSector = TRUE;
      break;
    }
  }

  // Double check that ...
  // if we are a EPC and are the selected guy, make single move off and disable
  // it....
  if (AM_AN_EPC(sel)) {
    // Check if there are more than one in this squad
    if (gExitDialog.ubNumPeopleOnSquad > 1) {
      gExitDialog.fSingleMoveOn = FALSE;
      gExitDialog.fAllMoveOn = TRUE;
      gExitDialog.fSelectedMercIsEPC = TRUE;
    }
    gExitDialog.fSingleMoveDisabled = TRUE;
  } else {  // check to see if we have one selected merc and one or more EPCs.
    // If so, don't allow the selected merc to leave by himself.
    // Assuming that the matching squad assignment is in the same sector.
    uint8_t ubNumMercs = 1;  // selected soldier is a merc
    uint8_t ubNumEPCs = 0;
    CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
      if (s == sel) continue;
      if (s->bAssignment == sel->bAssignment) {
        if (AM_AN_EPC(s)) {
          ubNumEPCs++;
          /* record the epc.  If there are more than one EPCs, then it doesn't
           * matter.  This is used in building the text message explaining why
           * the selected merc can't leave.  This is how we extract the EPC's
           * name. */
          gExitDialog.single_move_will_isolate_epc = s;
        } else {  // We have more than one merc, so we will allow the selected
                  // merc to leave alone if
          // the user so desired.
          ubNumMercs++;
          break;
        }
      }
    }

    if (ubNumMercs == 1 && ubNumEPCs >= 1) {
      gExitDialog.fSingleMoveOn = FALSE;
      gExitDialog.fAllMoveOn = TRUE;
      gExitDialog.fSingleMoveDisabled = TRUE;
      if (ubNumEPCs > 1) {
        gExitDialog.fSquadHasMultipleEPCs = TRUE;
      }
    }
  }

  if (gTacticalStatus.fEnemyInSector) {
    if (gExitDialog.fMultipleSquadsInSector) {  // We have multiple squads in a hostile
                                                // sector.  That means that we can't load
                                                // the adjacent sector.
      gExitDialog.fGotoSectorDisabled = TRUE;
      gExitDialog.fGotoSector = FALSE;
    } else if (GetNumberOfMilitiaInSector(gWorldSectorX, gWorldSectorY,
                                          gbWorldSectorZ)) {  // Leaving this sector will result in
                                                              // militia being forced to fight the
                                                              // battle, can't load adjacent sector.
      gExitDialog.fGotoSectorDisabled = TRUE;
      gExitDialog.fGotoSector = FALSE;
    }
    if (!gExitDialog.fMultipleSquadsInSector && !gExitDialog.fAllMoveOn) {
      gExitDialog.fGotoSectorDisabled = TRUE;
      gExitDialog.fGotoSector = FALSE;
    }
  }

  if (!gExitDialog.fMultipleSquadsInSector && gExitDialog.fAllMoveOn) {
    gExitDialog.fGotoSectorDisabled = TRUE;
  }

  gExitDialog.ubDirection = ubDirection;
  gExitDialog.sAdditionalData = sAdditionalData;

  gExitDialog.box = PrepareMercPopupBox(0, DIALOG_MERC_POPUP_BACKGROUND, DIALOG_MERC_POPUP_BORDER,
                                        TacticalStr[EXIT_GUI_TITLE_STR], 100, 85, 2, 75,
                                        &usTextBoxWidth, &usTextBoxHeight);

  gExitDialog.sX = (int16_t)((((aRect.iRight - aRect.iLeft) - usTextBoxWidth) / 2) + aRect.iLeft);
  gExitDialog.sY = (int16_t)((((aRect.iBottom - aRect.iTop) - usTextBoxHeight) / 2) + aRect.iTop);
  gExitDialog.usWidth = usTextBoxWidth;
  gExitDialog.usHeight = usTextBoxHeight;

  guiPendingOverrideEvent = EX_EXITSECTORMENU;
  HandleTacticalUI();

  gfInSectorExitMenu = TRUE;

  MSYS_DefineRegion(&gExitDialog.BackRegion, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                    MSYS_PRIORITY_HIGHEST - 1, CURSOR_NORMAL, MSYS_NO_CALLBACK,
                    SectorExitBackgroundCallback);

  gExitDialog.iButtonImages = LoadButtonImage(INTERFACEDIR "/popupbuttons.sti", 0, 1);

  MSYS_DefineRegion(&gExitDialog.SingleRegion, (int16_t)(gExitDialog.sX + 20),
                    (int16_t)(gExitDialog.sY + 37), (int16_t)(gExitDialog.sX + 45 + 120),
                    (int16_t)(gExitDialog.sY + 37 + 12), MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL,
                    SingleRegionMoveCallback, SingleRegionCallback);
  gExitDialog.SingleRegion.AllowDisabledRegionFastHelp(TRUE);

  MSYS_DefineRegion(&(gExitDialog.AllRegion), (int16_t)(gExitDialog.sX + 20),
                    (int16_t)(gExitDialog.sY + 57), (int16_t)(gExitDialog.sX + 45 + 120),
                    (int16_t)(gExitDialog.sY + 57 + 12), MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL,
                    AllRegionMoveCallback, AllRegionCallback);
  gExitDialog.AllRegion.AllowDisabledRegionFastHelp(TRUE);

  MSYS_DefineRegion(&(gExitDialog.LoadRegion), (int16_t)(gExitDialog.sX + 155),
                    (int16_t)(gExitDialog.sY + 45), (int16_t)(gExitDialog.sX + 180 + 85),
                    (int16_t)(gExitDialog.sY + 45 + 15), MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL,
                    LoadRegionMoveCallback, LoadRegionCallback);
  gExitDialog.LoadRegion.AllowDisabledRegionFastHelp(TRUE);

  gExitDialog.uiLoadCheckButton = CreateCheckBoxButton(
      (int16_t)(gExitDialog.sX + 155), (int16_t)(gExitDialog.sY + 43),
      INTERFACEDIR "/popupcheck.sti", MSYS_PRIORITY_HIGHEST, CheckLoadMapCallback);

  gExitDialog.uiSingleMoveButton = CreateCheckBoxButton(
      (int16_t)(gExitDialog.sX + 20), (int16_t)(gExitDialog.sY + 35),
      INTERFACEDIR "/popupradiobuttons.sti", MSYS_PRIORITY_HIGHEST, SingleMoveCallback);

  gExitDialog.uiAllMoveButton = CreateCheckBoxButton(
      (int16_t)(gExitDialog.sX + 20), (int16_t)(gExitDialog.sY + 55),
      INTERFACEDIR "/popupradiobuttons.sti", MSYS_PRIORITY_HIGHEST, AllMoveCallback);

  gExitDialog.uiOKButton = MakeButton(TacticalStr[OK_BUTTON_TEXT_STR], 65, OKCallback);
  gExitDialog.uiCancelButton = MakeButton(TacticalStr[CANCEL_BUTTON_TEXT_STR], 135, CancelCallback);

  gfIgnoreScrolling = TRUE;

  InterruptTime();
  PauseGame();
  LockPauseState(LOCK_PAUSE_21);
}

static void DoneFadeInWarp() {}

static void DoneFadeOutWarpCallback() {
  // Warp!
  FOR_EACH_IN_TEAM(pSoldier, OUR_TEAM) {
    // Are we in this sector, On the current squad?
    if (pSoldier->bLife >= OKLIFE && pSoldier->bInSector) {
      gfTacticalTraversal = TRUE;
      SetGroupSectorValue(gsWarpWorldX, gsWarpWorldY, gbWarpWorldZ, *GetGroup(pSoldier->ubGroupID));

      // Set next sectore
      pSoldier->sSectorX = gsWarpWorldX;
      pSoldier->sSectorY = gsWarpWorldY;
      pSoldier->bSectorZ = gbWarpWorldZ;

      // Set gridno
      pSoldier->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
      pSoldier->usStrategicInsertionData = gsWarpGridNo;
      // Set direction to face....
      pSoldier->ubInsertionDirection = 100 + NORTHWEST;
    }
  }

  // OK, insertion data found, enter sector!
  SetCurrentWorldSector(gsWarpWorldX, gsWarpWorldY, gbWarpWorldZ);

  // OK, once down here, adjust the above map with crate info....
  gfTacticalTraversal = FALSE;
  gpTacticalTraversalGroup = NULL;
  gpTacticalTraversalChosenSoldier = NULL;

  gFadeInDoneCallback = DoneFadeInWarp;

  FadeInGameScreen();
}

static void WarpToSurfaceCallback(MessageBoxReturnValue const bExitValue) {
  if (bExitValue == MSG_BOX_RETURN_YES) {
    gFadeOutDoneCallback = DoneFadeOutWarpCallback;

    FadeOutGameScreen();
  } else {
    InternalInitSectorExitMenu(gubExitGUIDirection, gsExitGUIAdditionalData);
  }
}

void InitSectorExitMenu(uint8_t const ubDirection, int16_t const sAdditionalData) {
  gubExitGUIDirection = ubDirection;
  gsExitGUIAdditionalData = sAdditionalData;

  if (gbWorldSectorZ >= 2 && gubQuest[QUEST_CREATURES] == QUESTDONE) {
    if (GetWarpOutOfMineCodes(&gsWarpWorldX, &gsWarpWorldY, &gbWarpWorldZ, &gsWarpGridNo)) {
      // ATE: Check if we are in a creature lair and bring up box if so....
      DoMessageBox(MSG_BOX_BASIC_STYLE, gzLateLocalizedString[STR_LATE_33], GAME_SCREEN,
                   MSG_BOX_FLAG_YESNO, WarpToSurfaceCallback, NULL);
      return;
    }
  }

  InternalInitSectorExitMenu(ubDirection, sAdditionalData);
}

static void UpdateSectorExitMenu() {
  if (gExitDialog.fGotoSector) {
    gExitDialog.uiLoadCheckButton->uiFlags |= BUTTON_CLICKED_ON;
  } else {
    gExitDialog.uiLoadCheckButton->uiFlags &= ~BUTTON_CLICKED_ON;
  }

  if (gExitDialog.fSingleMove) {
    gExitDialog.uiSingleMoveButton->uiFlags |= BUTTON_CLICKED_ON;
  } else {
    gExitDialog.uiSingleMoveButton->uiFlags &= ~BUTTON_CLICKED_ON;
  }

  if (gExitDialog.fAllMove) {
    gExitDialog.uiAllMoveButton->uiFlags |= BUTTON_CLICKED_ON;
  } else {
    gExitDialog.uiAllMoveButton->uiFlags &= ~BUTTON_CLICKED_ON;
  }

  {
    wchar_t const *help;
    if (gExitDialog.fGotoSectorDisabled) {
      DisableButton(gExitDialog.uiLoadCheckButton);
      gExitDialog.LoadRegion.Disable();
      if (!gExitDialog.fGotoSectorText) {  // Traversal takes too long to warrant
                                           // instant travel (we MUST go to mapscreen).
        help = pExitingSectorHelpText[EXIT_GUI_MUST_GOTO_MAPSCREEN_HELPTEXT];
      } else if (gExitDialog.fMultipleSquadsInSector &&
                 gTacticalStatus.fEnemyInSector) {  // We have multiple squads in a hostile
                                                    // sector.  That means that we can't
                                                    // load the adjacent sector.
        help = pExitingSectorHelpText[EXIT_GUI_CANT_LEAVE_HOSTILE_SECTOR_HELPTEXT];
      } else {  // Travesal is quick enough to allow the player to "warp" to the
                // next sector and we MUST load it.
        help = pExitingSectorHelpText[EXIT_GUI_MUST_LOAD_ADJACENT_SECTOR_HELPTEXT];
      }
    } else {
      EnableButton(gExitDialog.uiLoadCheckButton);
      gExitDialog.LoadRegion.Enable();
      if (gExitDialog.fGotoSectorText) {  // Travesal is quick enough to allow
                                          // the player to "warp" to the next
                                          // sector and we load it.
        help = pExitingSectorHelpText[EXIT_GUI_LOAD_ADJACENT_SECTOR_HELPTEXT];
      } else {  // Traversal takes too long to warrant instant travel (we go to
                // mapscreen)
        help = pExitingSectorHelpText[EXIT_GUI_GOTO_MAPSCREEN_HELPTEXT];
      }
    }
    gExitDialog.uiLoadCheckButton->SetFastHelpText(help);
    gExitDialog.LoadRegion.SetFastHelpText(help);
  }

  const SOLDIERTYPE *const sel = GetSelectedMan();
  if (gExitDialog.fSingleMoveDisabled) {
    DisableButton(gExitDialog.uiSingleMoveButton);
    gExitDialog.SingleRegion.Disable();
    if (gExitDialog.fSelectedMercIsEPC) {  // EPCs cannot leave the sector alone
                                           // and must be escorted
      wchar_t str[256];
      swprintf(str, lengthof(str),
               pExitingSectorHelpText[EXIT_GUI_ESCORTED_CHARACTERS_MUST_BE_ESCORTED_HELPTEXT],
               sel->name);
      gExitDialog.uiSingleMoveButton->SetFastHelpText(str);
      gExitDialog.SingleRegion.SetFastHelpText(str);
    } else if (gExitDialog.single_move_will_isolate_epc !=
               NULL) {  // It has been previously determined that there are only
                        // two mercs in the squad, the selected merc
      // isn't an EPC, but the other merc is.  That means that this merc cannot
      // leave the sector alone as he would isolate the EPC.
      wchar_t str[256];
      if (!gExitDialog.fSquadHasMultipleEPCs) {
        if (gMercProfiles[sel->ubProfile].bSex == MALE) {  // male singular
          swprintf(str, lengthof(str),
                   pExitingSectorHelpText[EXIT_GUI_MERC_CANT_ISOLATE_EPC_HELPTEXT_MALE_SINGULAR],
                   sel->name, gExitDialog.single_move_will_isolate_epc->name);
        } else {  // female singular
          swprintf(str, lengthof(str),
                   pExitingSectorHelpText[EXIT_GUI_MERC_CANT_ISOLATE_EPC_HELPTEXT_FEMALE_SINGULAR],
                   sel->name, gExitDialog.single_move_will_isolate_epc->name);
        }
      } else {
        if (gMercProfiles[sel->ubProfile].bSex == MALE) {  // male plural
          swprintf(str, lengthof(str),
                   pExitingSectorHelpText[EXIT_GUI_MERC_CANT_ISOLATE_EPC_HELPTEXT_MALE_PLURAL],
                   sel->name);
        } else {  // female plural
          swprintf(str, lengthof(str),
                   pExitingSectorHelpText[EXIT_GUI_MERC_CANT_ISOLATE_EPC_HELPTEXT_FEMALE_PLURAL],
                   sel->name);
        }
      }
      gExitDialog.uiSingleMoveButton->SetFastHelpText(str);
      gExitDialog.SingleRegion.SetFastHelpText(str);
    }
  } else {
    wchar_t str[256];
    EnableButton(gExitDialog.uiSingleMoveButton);
    gExitDialog.SingleRegion.Enable();
    swprintf(str, lengthof(str),
             pExitingSectorHelpText[EXIT_GUI_SINGLE_TRAVERSAL_WILL_SEPARATE_SQUADS_HELPTEXT],
             sel->name);
    gExitDialog.uiSingleMoveButton->SetFastHelpText(str);
    gExitDialog.SingleRegion.SetFastHelpText(str);
  }

  {
    wchar_t const *help;
    if (gExitDialog.fAllMoveDisabled) {
      DisableButton(gExitDialog.uiAllMoveButton);
      gExitDialog.AllRegion.Disable();
      help = gExitDialog.fUncontrolledRobotInSquad
                 ? gzLateLocalizedString[STR_LATE_01]
                 : pExitingSectorHelpText[EXIT_GUI_ALL_MERCS_MUST_BE_TOGETHER_TO_ALLOW_HELPTEXT];
    } else {
      EnableButton(gExitDialog.uiAllMoveButton);
      gExitDialog.AllRegion.Enable();
      help = pExitingSectorHelpText[EXIT_GUI_ALL_TRAVERSAL_WILL_MOVE_CURRENT_SQUAD_HELPTEXT];
    }
    gExitDialog.uiAllMoveButton->SetFastHelpText(help);
    gExitDialog.AllRegion.SetFastHelpText(help);
  }
}

void RenderSectorExitMenu() {
  RestoreBackgroundRects();
  // ATE: Reset mouse Y
  gsGlobalCursorYOffset = 0;
  SetCurrentCursorFromDatabase(CURSOR_NORMAL);

  InputAtom Event;
  while (DequeueEvent(&Event)) {
    if (Event.usEvent == KEY_DOWN) {
      switch (Event.usParam) {
        case SDLK_ESCAPE:
          RemoveSectorExitMenu(FALSE);
          return;
        case SDLK_RETURN:
          RemoveSectorExitMenu(TRUE);
          return;
      }
    }
  }

  UpdateSectorExitMenu();

  int16_t const x = gExitDialog.sX;
  int16_t const y = gExitDialog.sY;

  RenderMercPopUpBox(gExitDialog.box, x, y, FRAME_BUFFER);
  InvalidateRegion(x, y, gExitDialog.usWidth, gExitDialog.usHeight);

  SetFont(FONT12ARIAL);
  SetFontBackground(FONT_MCOLOR_BLACK);

  {
    uint8_t const foreground = gExitDialog.fSingleMoveDisabled    ? FONT_MCOLOR_DKGRAY
                               : gExitDialog.fSingleMoveHilighted ? FONT_MCOLOR_LTYELLOW
                                                                  : FONT_MCOLOR_WHITE;
    SetFontForeground(foreground);
    MPrint(x + 45, y + 37, TacticalStr[EXIT_GUI_SELECTED_MERC_STR]);
  }

  {
    uint8_t const foreground = gExitDialog.fAllMoveDisabled    ? FONT_MCOLOR_DKGRAY
                               : gExitDialog.fAllMoveHilighted ? FONT_MCOLOR_LTYELLOW
                                                               : FONT_MCOLOR_WHITE;
    SetFontForeground(foreground);
    MPrint(x + 45, y + 57, TacticalStr[EXIT_GUI_ALL_MERCS_IN_SQUAD_STR]);
  }

  {
    uint8_t const foreground = gExitDialog.fGotoSectorDisabled    ? FONT_MCOLOR_DKGRAY
                               : gExitDialog.fGotoSectorHilighted ? FONT_MCOLOR_LTYELLOW
                                                                  : FONT_MCOLOR_WHITE;
    SetFontForeground(foreground);
    wchar_t const *const msg = gExitDialog.fGotoSectorText
                                   ? TacticalStr[EXIT_GUI_GOTO_SECTOR_STR]
                                   :  // 5 minute convenience warp for town traversal
                                   TacticalStr[EXIT_GUI_GOTO_MAP_STR];  // Enter map screen
    MPrint(x + 180, y + 45, msg);
  }

  SaveBackgroundRects();
  RenderFastHelp();

  MarkAButtonDirty(gExitDialog.uiLoadCheckButton);
  MarkAButtonDirty(gExitDialog.uiSingleMoveButton);
  MarkAButtonDirty(gExitDialog.uiAllMoveButton);
  MarkAButtonDirty(gExitDialog.uiOKButton);
  MarkAButtonDirty(gExitDialog.uiCancelButton);
}

BOOLEAN HandleSectorExitMenu() {
  return (FALSE);  // Why???
}

void RemoveSectorExitMenu(BOOLEAN const fOk) {
  if (!gfInSectorExitMenu) return;
  gfInSectorExitMenu = FALSE;

  guiPendingOverrideEvent = A_CHANGE_TO_MOVE;

  EXIT_DIALOG_STRUCT &d = gExitDialog;
  RemoveButton(d.uiLoadCheckButton);
  RemoveButton(d.uiSingleMoveButton);
  RemoveButton(d.uiAllMoveButton);
  RemoveButton(d.uiOKButton);
  RemoveButton(d.uiCancelButton);

  UnloadButtonImage(d.iButtonImages);

  MSYS_RemoveRegion(&d.BackRegion);
  MSYS_RemoveRegion(&d.SingleRegion);
  MSYS_RemoveRegion(&d.AllRegion);
  MSYS_RemoveRegion(&d.LoadRegion);

  RemoveMercPopupBox(d.box);
  d.box = 0;

  gfIgnoreScrolling = FALSE;

  UnLockPauseState();
  UnPauseGame();
  EndModalTactical();

  if (!fOk) return;

  // If we are an EPC, don't allow this if nobody else on squad
  SOLDIERTYPE const *const sel = GetSelectedMan();
  if (AM_AN_EPC(sel) && d.ubNumPeopleOnSquad == 0) {
    wchar_t buf[50];
    swprintf(buf, lengthof(buf), pMessageStrings[MSG_EPC_CANT_TRAVERSE], sel->name);
    DoMessageBox(MSG_BOX_BASIC_STYLE, buf, GAME_SCREEN, MSG_BOX_FLAG_OK, 0, 0);
    return;
  }

  uint8_t jump_code;
  bool const do_load = d.fGotoSector && d.fGotoSectorText;
  if (d.fAllMove) {
    jump_code = do_load ? JUMP_ALL_LOAD_NEW : JUMP_ALL_NO_LOAD;
  } else if (d.fSingleMove) {
    jump_code = do_load ? JUMP_SINGLE_LOAD_NEW : JUMP_SINGLE_NO_LOAD;
  } else {
    return;
  }

  JumpIntoAdjacentSector(d.ubDirection, jump_code, d.sAdditionalData);
}

static void CheckLoadMapCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gExitDialog.fGotoSector = !gExitDialog.fGotoSector;
  }
}

static void SingleMoveAction() {
  // KM: New logic Mar2 '99
  if (!gExitDialog.fMultipleSquadsInSector) {
    if (gTacticalStatus.fEnemyInSector) {  // if enemy in sector, and mercs will be left
                                           // behind, prevent user from selecting load
      gExitDialog.fGotoSectorDisabled = TRUE;
      gExitDialog.fGotoSector = FALSE;
    } else {  // freedom to load or not load
      gExitDialog.fGotoSectorDisabled = FALSE;
    }
  } else {
    gExitDialog.fGotoSector = FALSE;
  }
  gExitDialog.fSingleMove = TRUE;
  gExitDialog.fAllMove = FALSE;
  // end

  // previous logic
  /*
  gExitDialog.fGotoSector = FALSE;
  gExitDialog.fSingleMove = TRUE;
  gExitDialog.fAllMove		= FALSE;
  */
}

static void AllMoveAction() {
  // KM: New logic Mar2 '99
  if (!gExitDialog.fMultipleSquadsInSector) {
    gExitDialog.fGotoSectorDisabled = TRUE;
    gExitDialog.fGotoSector = TRUE;
  }
  gExitDialog.fSingleMove = FALSE;
  gExitDialog.fAllMove = TRUE;
  // end

  // previous logic
  /*
  gExitDialog.fSingleMove = FALSE;
  gExitDialog.fAllMove		= TRUE;
  */
}

static void SingleMoveCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    SingleMoveAction();
  }
}

static void AllMoveCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    AllMoveAction();
  }
}

static void OKCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // OK, exit
    RemoveSectorExitMenu(TRUE);
  }
}

static void CancelCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // OK, exit
    RemoveSectorExitMenu(FALSE);
  }
}

static void SectorExitBackgroundCallback(MOUSE_REGION *pRegion, int32_t iReason) {}

static void SingleRegionCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    SingleMoveAction();
  }
}

static void AllRegionCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    AllMoveAction();
  }
}

static void LoadRegionCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gExitDialog.fGotoSector = !gExitDialog.fGotoSector;
  }
}

static void SingleRegionMoveCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_MOVE) {
    gExitDialog.fSingleMoveHilighted = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    gExitDialog.fSingleMoveHilighted = FALSE;
  }
}

static void AllRegionMoveCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_MOVE) {
    gExitDialog.fAllMoveHilighted = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    gExitDialog.fAllMoveHilighted = FALSE;
  }
}

static void LoadRegionMoveCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_MOVE) {
    gExitDialog.fGotoSectorHilighted = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    gExitDialog.fGotoSectorHilighted = FALSE;
  }
}
