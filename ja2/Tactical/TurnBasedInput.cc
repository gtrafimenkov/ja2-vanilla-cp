// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/TurnBasedInput.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Editor/EditScreen.h"
#include "GameLoop.h"
#include "JAScreens.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "OptionsScreen.h"
#include "SGP/CursorControl.h"
#include "SGP/English.h"
#include "SGP/Input.h"
#include "SGP/Random.h"
#include "ScreenIDs.h"
#include "Strategic/Assignments.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEvents.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/QuestDebugSystem.h"
#include "Strategic/StrategicMap.h"
#include "SysGlobals.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/AnimationData.h"
#include "Tactical/AutoBandage.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/HandleItems.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceCursors.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/MercEntering.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/Points.h"
#include "Tactical/RealTimeInput.h"
#include "Tactical/SoldierAdd.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/SoldierFind.h"
#include "Tactical/SoldierFunctions.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/SoldierTile.h"
#include "Tactical/SpreadBurst.h"
#include "Tactical/Squads.h"
#include "Tactical/StrategicExitGUI.h"
#include "Tactical/StructureWrap.h"
#include "Tactical/UICursors.h"
#include "Tactical/Weapons.h"
#include "Tactical/WorldItems.h"
#include "TacticalAI/AI.h"
#include "TileEngine/Environment.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/InteractiveTiles.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/OverheadMap.h"
#include "TileEngine/Physics.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileAnimation.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldMan.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/MusicControl.h"
#include "Utils/SoundControl.h"
#include "Utils/TimerControl.h"

#include "SDL_keycode.h"
// #include "medical.h"
#include "Cheats.h"
#include "GameRes.h"
#include "GameSettings.h"
#include "GameState.h"
#include "HelpScreen.h"
#include "Laptop/History.h"
#include "SGP/ButtonSystem.h"
#include "SGP/MemMan.h"
#include "SGP/VSurface.h"
#include "SaveLoadScreen.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/StrategicStatus.h"
#include "Tactical/AirRaid.h"
#include "Tactical/ArmsDealerInit.h"
#include "Tactical/DisplayCover.h"
#include "Tactical/InventoryChoosing.h"
#include "Tactical/Items.h"
#include "Tactical/ShopKeeperInterface.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/Vehicles.h"
#include "Utils/Text.h"

#ifdef JA2TESTVERSION
#include "Ambient_Control.h"
#include "MapScreen.h"
#endif

#ifdef JA2BETAVERSION
#include "Strategic_AI.h"
#endif

#ifdef SGP_VIDEO_DEBUGGING
#include "VObject.h"
#endif

static BOOLEAN gfFirstCycleMovementStarted = FALSE;

const SOLDIERTYPE *gUITargetSoldier = NULL;

static void QueryTBLeftButton(UIEventKind *);
static void QueryTBRightButton(UIEventKind *);

void GetTBMouseButtonInput(UIEventKind *const puiNewEvent) {
  QueryTBLeftButton(puiNewEvent);
  QueryTBRightButton(puiNewEvent);
}

static void QueryTBLeftButton(UIEventKind *const puiNewEvent) {
  static BOOLEAN fClickHoldIntercepted = FALSE;
  static BOOLEAN fCanCheckForSpeechAdvance = FALSE;

  // LEFT MOUSE BUTTON
  if (gViewportRegion.uiFlags & MSYS_MOUSE_IN_AREA) {
    const GridNo usMapPos = GetMouseMapPos();
    if (usMapPos == NOWHERE && !gfUIShowExitSouth) return;

    if (gViewportRegion.ButtonState & MSYS_LEFT_BUTTON) {
      if (!fLeftButtonDown) {
        fLeftButtonDown = TRUE;
        RESETCOUNTER(LMOUSECLICK_DELAY_COUNTER);

        {
          switch (gCurrentUIMode) {
            case CONFIRM_ACTION_MODE: {
              SOLDIERTYPE *const sel = GetSelectedMan();
              if (sel != NULL) sel->sStartGridNo = usMapPos;
              break;
            }

            case MOVE_MODE:
              if (g_ui_message_overlay != NULL) {
                EndUIMessage();
              } else {
                if (!HandleCheckForExitArrowsInput(FALSE) && gpItemPointer == NULL) {
                  // First check if we clicked on a guy, if so, make selected if
                  // it's ours
                  const SOLDIERTYPE *const tgt = gUIFullTarget;
                  if (tgt != NULL && guiUIFullTargetFlags & OWNED_MERC) {
                    if (!(guiUIFullTargetFlags & UNCONSCIOUS_MERC)) {
                      fClickHoldIntercepted = TRUE;

                      // Select guy
                      if (gpItemPointer == NULL) {
                        if (tgt->bAssignment >= ON_DUTY) {
                          // do nothing
                          fClickHoldIntercepted = FALSE;
                        } else {
                          *puiNewEvent = I_SELECT_MERC;
                        }
                      } else {
                        *puiNewEvent = I_SELECT_MERC;
                      }
                    }
                  } else {
                    // We're on terrain in which we can walk, walk
                    // If we're on terrain,
                    const SOLDIERTYPE *const sel = GetSelectedMan();
                    if (sel != NULL) {
                      int8_t const bReturnVal = HandleMoveModeInteractiveClick(usMapPos);
                      // All's OK for interactive tile?
                      if (bReturnVal == -2) {
                        // Confirm!
                        if (SelectedMercCanAffordMove()) {
                          *puiNewEvent = C_WAIT_FOR_CONFIRM;
                        }
                      } else if (bReturnVal == 0) {
                        if (gfUIAllMoveOn) {
                          *puiNewEvent = C_WAIT_FOR_CONFIRM;
                        } else {
                          if (gsCurrentActionPoints == 0) {
                            ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NO_PATH]);
                          } else if (SelectedMercCanAffordMove()) {
                            const BOOLEAN fResult = UIOKMoveDestination(sel, usMapPos);
                            if (fResult == 1) {
                              // ATE: CHECK IF WE CAN GET TO POSITION
                              // Check if we are not in combat
                              if (gsCurrentActionPoints == 0) {
                                ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK,
                                          TacticalStr[NO_PATH]);
                              } else {
                                *puiNewEvent = C_WAIT_FOR_CONFIRM;
                              }
                            } else if (fResult == 2) {
                              ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK,
                                        TacticalStr[NOBODY_USING_REMOTE_STR]);
                            }
                          }
                        }
                      }
                      // OK, our first right-click is an all-cycle
                      gfUICanBeginAllMoveCycle = FALSE;
                    }
                    fClickHoldIntercepted = TRUE;
                  }
                } else {
                  fClickHoldIntercepted = TRUE;
                  fIgnoreLeftUp = TRUE;
                }
              }
              break;
            default:
              break;
          }
        }
        if (gfUIWaitingForUserSpeechAdvance) {
          fCanCheckForSpeechAdvance = TRUE;
        }
      }

      // HERE FOR CLICK-DRAG CLICK
      switch (gCurrentUIMode) {
        case MOVE_MODE:
          // First check if we clicked on a guy, if so, make selected if it's ours
          if (gUIFullTarget != NULL && guiUIFullTargetFlags & SELECTED_MERC &&
              !(guiUIFullTargetFlags & UNCONSCIOUS_MERC) &&
              !(gUIFullTarget->uiStatusFlags & SOLDIER_VEHICLE)) {
            *puiNewEvent = M_CHANGE_TO_ADJPOS_MODE;
            fIgnoreLeftUp = FALSE;
          }
          break;
        default:
          break;
      }
    } else {
      if (fLeftButtonDown) {
        if (!fIgnoreLeftUp) {
          // FIRST CHECK FOR ANYTIME ( NON-INTERVAL ) CLICKS
          switch (gCurrentUIMode) {
            case ADJUST_STANCE_MODE:

              // If button has come up, change to mocve mode
              *puiNewEvent = PADJ_ADJUST_STANCE;
              break;
            default:
              break;
          }

          // CHECK IF WE CLICKED-HELD
          if (COUNTERDONE(LMOUSECLICK_DELAY_COUNTER)) {
            // LEFT CLICK-HOLD EVENT
            // Switch on UI mode
            switch (gCurrentUIMode) {
              case CONFIRM_ACTION_MODE: {
                SOLDIERTYPE *const sel = GetSelectedMan();
                if (sel != NULL && sel->bDoBurst) {
                  sel->sEndGridNo = usMapPos;

                  gfBeginBurstSpreadTracking = FALSE;

                  if (sel->sEndGridNo != sel->sStartGridNo) {
                    sel->fDoSpread = TRUE;
                    PickBurstLocations(sel);
                    *puiNewEvent = CA_MERC_SHOOT;
                  } else {
                    sel->fDoSpread = FALSE;
                  }

                  fClickHoldIntercepted = TRUE;
                }
                break;
              }
              default:
                break;
            }
          }

          {
            // LEFT CLICK NORMAL EVENT
            // Switch on UI mode
            if (!fClickHoldIntercepted) {
              if (g_ui_message_overlay != NULL) {
                EndUIMessage();
              } else {
                if (!HandleCheckForExitArrowsInput(TRUE)) {
                  if (gpItemPointer != NULL) {
                    if (HandleItemPointerClick(usMapPos)) {
                      // getout of mode
                      EndItemPointer();

                      *puiNewEvent = A_CHANGE_TO_MOVE;
                    }
                  } else {
                    // Check for wiating for keyboard advance
                    if (gfUIWaitingForUserSpeechAdvance && fCanCheckForSpeechAdvance) {
                      // We have a key, advance!
                      DialogueAdvanceSpeech();
                    } else {
                      switch (gCurrentUIMode) {
                        case MENU_MODE:

                          // If we get a hit here and we're in menu mode, quit the
                          // menu mode
                          EndMenuEvent(guiCurrentEvent);
                          break;

                        case IDLE_MODE:

                          // First check if we clicked on a guy, if so, make
                          // selected if it's ours
                          if (gUIFullTarget != NULL && guiUIFullTargetFlags & OWNED_MERC &&
                              !(guiUIFullTargetFlags & UNCONSCIOUS_MERC)) {
                            *puiNewEvent = I_SELECT_MERC;
                          }
                          break;

                        case MOVE_MODE:

                          // Check if we should activate an interactive tile!
                          // Select guy
                          if ((guiUIFullTargetFlags & OWNED_MERC) &&
                              !(guiUIFullTargetFlags & UNCONSCIOUS_MERC)) {
                            const SOLDIERTYPE *const tgt = gUIFullTarget;
                            if (tgt != NULL && gpItemPointer == NULL &&
                                !(tgt->uiStatusFlags & SOLDIER_VEHICLE) &&
                                tgt->bAssignment >= ON_DUTY) {
                              PopupAssignmentMenuInTactical();
                            }
                          }
                          break;

                        case CONFIRM_MOVE_MODE:

                          *puiNewEvent = C_MOVE_MERC;
                          break;

                        case HANDCURSOR_MODE:

                          HandleHandCursorClick(usMapPos, puiNewEvent);
                          break;

                        case JUMPOVER_MODE: {
                          const SOLDIERTYPE *const sel = GetSelectedMan();
                          if (sel != NULL && EnoughPoints(sel, gsCurrentActionPoints, 0, TRUE)) {
                            *puiNewEvent = JP_JUMP;
                          }
                          break;
                        }

                        case ACTION_MODE: {
                          SOLDIERTYPE *const sel = GetSelectedMan();
                          if (sel != NULL && !HandleUIReloading(sel)) {
                            // ATE: Reset refine aim..
                            sel->bShownAimTime = 0;

                            if (gsCurrentActionPoints == 0) {
                              ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK,
                                        TacticalStr[NO_PATH]);
                            }
                            // Determine if we have enough action points!
                            else if (UIMouseOnValidAttackLocation(sel) &&
                                     SelectedMercCanAffordAttack()) {
                              *puiNewEvent = A_CHANGE_TO_CONFIM_ACTION;
                              sel->sStartGridNo = usMapPos;
                            }
                          }
                          break;
                        }

                        case CONFIRM_ACTION_MODE:

                          *puiNewEvent = CA_MERC_SHOOT;
                          break;

                        case LOOKCURSOR_MODE:
                          // If we cannot actually do anything, return to movement
                          // mode
                          *puiNewEvent = LC_LOOK;
                          break;

                        case TALKCURSOR_MODE:

                          if (HandleTalkInit()) {
                            *puiNewEvent = TA_TALKINGMENU;
                          }
                          break;

                        case GETTINGITEM_MODE:

                          // Remove menu!
                          // RemoveItemPickupMenu( );
                          break;

                        case TALKINGMENU_MODE:

                          // HandleTalkingMenuEscape( TRUE );
                          break;

                        case EXITSECTORMENU_MODE:

                          RemoveSectorExitMenu(FALSE);
                          break;

                        case OPENDOOR_MENU_MODE:

                          CancelOpenDoorMenu();
                          HandleOpenDoorMenu();
                          *puiNewEvent = A_CHANGE_TO_MOVE;
                          break;

                        default:
                          break;
                      }
                    }
                  }
                }
              }
            }
          }
        }

        // Reset flag
        fLeftButtonDown = FALSE;
        fIgnoreLeftUp = FALSE;
        fClickHoldIntercepted = FALSE;
        fCanCheckForSpeechAdvance = FALSE;
        gfFirstCycleMovementStarted = FALSE;

        // Reset counter
        RESETCOUNTER(LMOUSECLICK_DELAY_COUNTER);
      }
    }

  } else {
    // Set mouse down to false
    // fLeftButtonDown = FALSE;

    // OK, handle special cases like if we are dragging and holding for a burst
    // spread and release mouse over another mouse region
    if (gfBeginBurstSpreadTracking) {
      SOLDIERTYPE *const sel = GetSelectedMan();
      if (sel != NULL) sel->fDoSpread = FALSE;
      gfBeginBurstSpreadTracking = FALSE;
    }
  }
}

static void QueryTBRightButton(UIEventKind *const puiNewEvent) {
  static BOOLEAN fClickHoldIntercepted = FALSE;
  static BOOLEAN fClickIntercepted = FALSE;

  const GridNo usMapPos = GetMouseMapPos();
  if (usMapPos == NOWHERE) return;

  if (gViewportRegion.uiFlags & MSYS_MOUSE_IN_AREA) {
    // RIGHT MOUSE BUTTON
    if (gViewportRegion.ButtonState & MSYS_RIGHT_BUTTON) {
      if (!fRightButtonDown) {
        fRightButtonDown = TRUE;
        RESETCOUNTER(RMOUSECLICK_DELAY_COUNTER);
      }

      // CHECK COMBINATIONS
      if (fLeftButtonDown) {
      } else {
        if (gpItemPointer == NULL) {
          // IF HERE, DO A CLICK-HOLD IF IN INTERVAL
          if (COUNTERDONE(RMOUSECLICK_DELAY_COUNTER) && !fClickHoldIntercepted) {
            // Switch on UI mode
            switch (gCurrentUIMode) {
              case IDLE_MODE:
              case ACTION_MODE:
              case HANDCURSOR_MODE:
              case LOOKCURSOR_MODE:
              case TALKCURSOR_MODE:
              case MOVE_MODE:
                if (guiUIFullTargetFlags & OWNED_MERC &&
                    !(guiUIFullTargetFlags & UNCONSCIOUS_MERC)) {
                  const SOLDIERTYPE *const tgt = gUIFullTarget;
                  if (tgt != NULL && gpItemPointer == NULL &&
                      !(tgt->uiStatusFlags & SOLDIER_VEHICLE)) {
                    // if (tgt->bAssignment >= ON_DUTY)
                    {
                      PopupAssignmentMenuInTactical();
                      fClickHoldIntercepted = TRUE;
                    }
                  }
                }

                if (GetSelectedMan() != NULL && !fClickHoldIntercepted) {
                  *puiNewEvent = U_MOVEMENT_MENU;
                  fClickHoldIntercepted = TRUE;
                }
                break;
              default:
                break;
            }
          }
        }
      }

    } else {
      if (fRightButtonDown) {
        if (fLeftButtonDown) {
          fIgnoreLeftUp = TRUE;

          if (gpItemPointer == NULL) {
            // ATE:
            SOLDIERTYPE *const sel = GetSelectedMan();
            if (sel != NULL) {
              switch (gCurrentUIMode) {
                case CONFIRM_MOVE_MODE:
                case MOVE_MODE:

                  if (gfUICanBeginAllMoveCycle) {
                    *puiNewEvent = M_CYCLE_MOVE_ALL;
                  } else {
                    if (!gfFirstCycleMovementStarted) {
                      gfFirstCycleMovementStarted = TRUE;

                      // OK, set this guy's movement mode to crawling fo rthat we
                      // will start cycling in run.....
                      if (sel->usUIMovementMode != RUNNING) {
                        // ATE: UNLESS WE ARE IN RUNNING MODE ALREADY
                        sel->usUIMovementMode = CRAWLING;
                      }
                    }

                    // Give event to cycle movement
                    *puiNewEvent = M_CYCLE_MOVEMENT;
                    break;
                  }
                default:
                  break;
              }

              // ATE: Added cancel of burst mode....
              if (gfBeginBurstSpreadTracking) {
                gfBeginBurstSpreadTracking = FALSE;
                gfRTClickLeftHoldIntercepted = TRUE;
                sel->fDoSpread = FALSE;
                fClickHoldIntercepted = TRUE;
                *puiNewEvent = A_END_ACTION;
                gCurrentUIMode = MOVE_MODE;
              }
            }
          }
        } else {
          if (!fClickHoldIntercepted && !fClickIntercepted) {
            if (gpItemPointer == NULL) {
              // ATE:
              SOLDIERTYPE *const sel = GetSelectedMan();
              if (sel != NULL) {
                // Switch on UI mode
                switch (gCurrentUIMode) {
                  case IDLE_MODE:

                    break;

                  case MOVE_MODE:

                    // We have here a change to action mode
                    *puiNewEvent = M_CHANGE_TO_ACTION;
                    fClickIntercepted = TRUE;
                    break;

                  case ACTION_MODE:

                    // We have here a change to action mode
                    *puiNewEvent = A_CHANGE_TO_MOVE;
                    fClickIntercepted = TRUE;
                    break;

                  case CONFIRM_MOVE_MODE:

                    *puiNewEvent = A_CHANGE_TO_MOVE;
                    fClickIntercepted = TRUE;
                    break;

                  case HANDCURSOR_MODE:
                    // If we cannot actually do anything, return to movement mode
                    *puiNewEvent = A_CHANGE_TO_MOVE;
                    break;

                  case LOOKCURSOR_MODE:
                  case TALKCURSOR_MODE:

                    // If we cannot actually do anything, return to movement mode
                    *puiNewEvent = A_CHANGE_TO_MOVE;
                    break;

                  case CONFIRM_ACTION_MODE:
                    HandleRightClickAdjustCursor(sel, usMapPos);
                    fClickIntercepted = TRUE;
                    break;

                  case MENU_MODE:

                    // If we get a hit here and we're in menu mode, quit the menu
                    // mode
                    EndMenuEvent(guiCurrentEvent);
                    fClickIntercepted = TRUE;
                    break;
                  default:
                    break;
                }
              }
            } else {
              if (gUIFullTarget != NULL) {
                gfItemPointerDifferentThanDefault = !gfItemPointerDifferentThanDefault;
              }
            }
          }
        }
      }
      // Reset flag
      fRightButtonDown = FALSE;
      fClickHoldIntercepted = FALSE;
      fClickIntercepted = FALSE;

      // Reset counter
      RESETCOUNTER(RMOUSECLICK_DELAY_COUNTER);
    }
  }
}

void GetTBMousePositionInput(UIEventKind *const puiNewEvent) {
  static const SOLDIERTYPE *MoveTargetSoldier = NULL;

  static uint16_t usOldMapPos = 0;
  static BOOLEAN fOnValidGuy = FALSE;

  const GridNo usMapPos = GetMouseMapPos();
  if (usMapPos == NOWHERE) return;

  if (gViewportRegion.uiFlags & MSYS_MOUSE_IN_AREA) {
    // Check if we have an item in our hands...
    if (gpItemPointer != NULL) {
      *puiNewEvent = A_ON_TERRAIN;
      return;
    }

    // Switch on modes
    switch (gCurrentUIMode) {
      case LOCKUI_MODE:
        *puiNewEvent = LU_ON_TERRAIN;
        break;

      case LOCKOURTURN_UI_MODE:
        *puiNewEvent = LA_ON_TERRAIN;
        break;

      case IDLE_MODE:
        *puiNewEvent = I_ON_TERRAIN;
        break;

      case ENEMYS_TURN_MODE:
        *puiNewEvent = ET_ON_TERRAIN;
        break;

      case LOOKCURSOR_MODE:
        *puiNewEvent = LC_ON_TERRAIN;
        break;

      case TALKCURSOR_MODE:
        if (MoveTargetSoldier != NULL && gUIFullTarget != MoveTargetSoldier) {
          *puiNewEvent = A_CHANGE_TO_MOVE;
          return;
        }
        *puiNewEvent = T_ON_TERRAIN;
        break;

      case MOVE_MODE: {
        MoveTargetSoldier = NULL;

        // Check for being on terrain
        const SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel != NULL) {
          if (IsValidJumpLocation(sel, usMapPos, TRUE)) {
            gsJumpOverGridNo = usMapPos;
            *puiNewEvent = JP_ON_TERRAIN;
            return;
          } else {
            const SOLDIERTYPE *const tgt = gUIFullTarget;
            if (tgt != NULL) {
              // ATE: Don't do this automatically for enemies......
              if (tgt->bTeam != ENEMY_TEAM) {
                MoveTargetSoldier = tgt;
                if (IsValidTalkableNPC(tgt, FALSE, FALSE, FALSE) && !IsKeyDown(SHIFT) &&
                    !AM_AN_EPC(sel) && !ValidQuickExchangePosition()) {
                  *puiNewEvent = T_CHANGE_TO_TALKING;
                  return;
                }
              }
            }
          }
        }
        *puiNewEvent = M_ON_TERRAIN;
        break;
      }

      case ACTION_MODE: {
        // First check if we are on a guy, if so, make selected if it's ours
        // Check if the guy is visible
        gUITargetSoldier = NULL;

        fOnValidGuy = FALSE;

        const SOLDIERTYPE *const tgt = gUIFullTarget;
        if (tgt != NULL) {
          if (IsValidTargetMerc(tgt)) {
            gUITargetSoldier = tgt;

            if (tgt->bTeam != OUR_TEAM) {
              fOnValidGuy = TRUE;
            } else {
              if (gUIActionModeChangeDueToMouseOver) {
                *puiNewEvent = A_CHANGE_TO_MOVE;
                return;
              }
            }
          }
        } else {
          if (gUIActionModeChangeDueToMouseOver) {
            *puiNewEvent = A_CHANGE_TO_MOVE;
            return;
          }
        }
        *puiNewEvent = A_ON_TERRAIN;
        break;
      }

      case GETTINGITEM_MODE:

        break;

      case TALKINGMENU_MODE:

        if (HandleTalkingMenu()) {
          *puiNewEvent = A_CHANGE_TO_MOVE;
        }
        break;

      case EXITSECTORMENU_MODE:

        if (HandleSectorExitMenu()) {
          *puiNewEvent = A_CHANGE_TO_MOVE;
        }
        break;

      case OPENDOOR_MENU_MODE: {
        BOOLEAN const bHandleCode = HandleOpenDoorMenu();
        // If we are not canceling, set UI back!
        if (bHandleCode == 2) {
          *puiNewEvent = A_CHANGE_TO_MOVE;
        }
        break;
      }

      case JUMPOVER_MODE:

        // ATE: Make sure!
        if (gsJumpOverGridNo != usMapPos) {
          *puiNewEvent = A_CHANGE_TO_MOVE;
        } else {
          *puiNewEvent = JP_ON_TERRAIN;
        }
        break;

      case CONFIRM_MOVE_MODE:

        if (usMapPos != usOldMapPos) {
          // Switch event out of confirm mode
          *puiNewEvent = A_CHANGE_TO_MOVE;

          // Set off ALL move....
          gfUIAllMoveOn = FALSE;

          // ERASE PATH
          ErasePath();
        }
        break;

      case CONFIRM_ACTION_MODE: {
        // DONOT CANCEL IF BURST
        SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel != NULL && sel->bDoBurst) {
          sel->sEndGridNo = usMapPos;

          if (sel->sEndGridNo != sel->sStartGridNo && fLeftButtonDown) {
            sel->fDoSpread = TRUE;
            gfBeginBurstSpreadTracking = TRUE;
          }

          if (sel->fDoSpread) {
            // Accumulate gridno
            AccumulateBurstLocation(usMapPos);

            *puiNewEvent = CA_ON_TERRAIN;
            break;
          }
        }

        // First check if we are on a guy, if so, make selected if it's ours
        if (gUIFullTarget != NULL) {
          if (gUITargetSoldier != gUIFullTarget) {
            // Switch event out of confirm mode
            *puiNewEvent = CA_END_CONFIRM_ACTION;
          } else {
            *puiNewEvent = CA_ON_TERRAIN;
          }
        } else {
          // OK, if we were on a guy, and now we are off, go back!
          if (fOnValidGuy) {
            // Switch event out of confirm mode
            *puiNewEvent = CA_END_CONFIRM_ACTION;
          } else {
            if (ConfirmActionCancel(usMapPos, usOldMapPos)) {
              // Switch event out of confirm mode
              *puiNewEvent = CA_END_CONFIRM_ACTION;
            } else {
              *puiNewEvent = CA_ON_TERRAIN;
            }
          }
        }
        break;
      }

      case HANDCURSOR_MODE:

        *puiNewEvent = HC_ON_TERRAIN;
        break;

      default:
        break;
    }

    usOldMapPos = usMapPos;
  }
}

void GetPolledKeyboardInput(UIEventKind *puiNewEvent) {
  // static BOOLEAN fShifted = FALSE;
  static BOOLEAN fShifted2 = FALSE;
  static BOOLEAN fCtrlDown = FALSE;
  static BOOLEAN fAltDown = FALSE;
  static BOOLEAN fDeleteDown = FALSE;
  static BOOLEAN fEndDown = FALSE;

  // CHECK FOR POLLED KEYS!!
  // CHECK FOR CTRL
  switch (gCurrentUIMode) {
    case DONT_CHANGEMODE:
    case CONFIRM_MOVE_MODE:
    case CONFIRM_ACTION_MODE:
    case LOOKCURSOR_MODE:
    case TALKCURSOR_MODE:
    case IDLE_MODE:
    case MOVE_MODE:
    case ACTION_MODE:
    case HANDCURSOR_MODE:

      if (IsKeyDown(CTRL)) {
        if (!fCtrlDown) {
          ErasePath();
          gfPlotNewMovement = TRUE;
        }
        fCtrlDown = TRUE;
        *puiNewEvent = HC_ON_TERRAIN;
      }
      if (!(IsKeyDown(CTRL)) && fCtrlDown) {
        fCtrlDown = FALSE;
        *puiNewEvent = M_ON_TERRAIN;
        gfPlotNewMovement = TRUE;
      }
      break;
    default:
      break;
  }

  // CHECK FOR ALT
  switch (gCurrentUIMode) {
    case MOVE_MODE:

      if (IsKeyDown(ALT)) {
        if (!fAltDown) {
          // Get currently selected guy and change reverse....
          if (GetSelectedMan() != NULL) {
            gUIUseReverse = TRUE;
            ErasePath();
            gfPlotNewMovement = TRUE;
          }
        }
        fAltDown = TRUE;
      }
      if (!IsKeyDown(ALT) && fAltDown) {
        if (GetSelectedMan() != NULL) {
          gUIUseReverse = FALSE;
          ErasePath();
          gfPlotNewMovement = TRUE;
        }

        fAltDown = FALSE;
      }
      break;
    default:
      break;
  }

  // Check realtime input!
  if (((gTacticalStatus.uiFlags & REALTIME) || !(gTacticalStatus.uiFlags & INCOMBAT))) {
    // if (IsKeyDown(SDLK_CAPSLOCK)) //&& !fShifted)
    //{
    //	fShifted = TRUE;
    //	if ( gCurrentUIMode != ACTION_MODE && gCurrentUIMode !=
    // CONFIRM_ACTION_MODE )
    //	{
    //		*puiNewEvent = CA_ON_TERRAIN;
    //	}
    //}
    // if (!(IsKeyDown(SDLK_CAPSLOCK)) && fShifted)
    //{
    //	fShifted = FALSE;
    //	{
    //		*puiNewEvent = M_ON_TERRAIN;
    //	}
    //}

    if (IsKeyDown(SHIFT))  //&& !fShifted )
    {
      fShifted2 = TRUE;
      if (gCurrentUIMode != MOVE_MODE && gCurrentUIMode != CONFIRM_MOVE_MODE) {
        // puiNewEvent = M_ON_TERRAIN;
      }
    }
    if (!(IsKeyDown(SHIFT)) && fShifted2) {
      fShifted2 = FALSE;
      if (gCurrentUIMode != ACTION_MODE && gCurrentUIMode != CONFIRM_ACTION_MODE) {
        //	*puiNewEvent = A_ON_TERRAIN;
      }
    }
  }

  if (IsKeyDown(SDLK_DELETE)) {
    DisplayCoverOfSelectedGridNo();

    fDeleteDown = TRUE;
  }

  if (!IsKeyDown(SDLK_DELETE) && fDeleteDown) {
    RemoveCoverOfSelectedGridNo();

    fDeleteDown = FALSE;
  }

  if (IsKeyDown(SDLK_END)) {
    DisplayGridNoVisibleToSoldierGrid();

    fEndDown = TRUE;
  }

  if (!IsKeyDown(SDLK_END) && fEndDown) {
    RemoveVisibleGridNoAtSelectedGridNo();

    fEndDown = FALSE;
  }
}

static void ChangeCurrentSquad(int32_t iSquad);
static void ChangeSoldiersBodyType(SoldierBodyType, BOOLEAN fCreateNewPalette);
static void CreateNextCivType();
static void CreatePlayerControlledMonster();
static void CreateRandomItem();
static void CycleSelectedMercsItem();
static void EscapeUILock();
static void GrenadeTest1();
static void GrenadeTest2();
static void HandleItemMenuKeys(InputAtom *, UIEventKind *);
static void HandleMenuKeys(InputAtom *, UIEventKind *);
static void HandleOpenDoorMenuKeys(InputAtom *, UIEventKind *);
static void HandleSectorExitMenuKeys(InputAtom *, UIEventKind *);
static void HandleSelectMercSlot(uint8_t ubPanelSlot, bool force_select);
static void HandleStealthChangeFromUIKeys();
static void HandleTalkingMenuKeys(InputAtom *, UIEventKind *);
static void ObliterateSector();
static void RefreshSoldier();
static void SetBurstMode();
static void TeleportSelectedSoldier();
static void TestCapture();
static void TestMeanWhile(int32_t iID);
static void ToggleCliffDebug();
static void ToggleTreeTops();
static void ToggleViewAllItems();
static void ToggleViewAllMercs();
static void ToggleWireFrame();
static void ToggleZBuffer();

static void ToggleMapEdgepoints();

static void HandleModNone(uint32_t const key, UIEventKind *const new_event) {
  switch (key) {
    case SDLK_TAB:
      // Nothing in hand and either not in SM panel, or the matching button is
      // enabled if we are in SM panel
      if (!gpItemPointer &&
          (gsCurInterfacePanel != SM_PANEL || iSMPanelButtons[UPDOWN_BUTTON]->Enabled())) {
        UIHandleChangeLevel(0);
      }
      break;

    case SDLK_SPACE:
      // Nothing in hand and either not in SM panel, or the matching button is
      // enabled if we are in SM panel
      if (!(gTacticalStatus.uiFlags & ENGAGED_IN_CONV) &&
          (gsCurInterfacePanel != SM_PANEL || iSMPanelButtons[NEXTMERC_BUTTON]->Enabled()) &&
          !InKeyRingPopup()) {
        SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel) {
          // Select next merc
          SOLDIERTYPE *const next = FindNextMercInTeamPanel(sel);
          HandleLocateSelectMerc(next, true);
          // Center to guy
          LocateSoldier(GetSelectedMan(), SETLOCATOR);
        }

        *new_event = M_ON_TERRAIN;
      }
      break;

    case '*':
      gTacticalStatus.uiFlags ^= RED_ITEM_GLOW_ON;
      break;

    case '-':
      // If the display cover or line of sight is being displayed
      if (IsKeyDown(SDLK_END) || IsKeyDown(SDLK_DELETE)) {
        if (IsKeyDown(SDLK_DELETE))
          ChangeSizeOfDisplayCover(gGameSettings.ubSizeOfDisplayCover - 1);
        if (IsKeyDown(SDLK_END)) ChangeSizeOfLOS(gGameSettings.ubSizeOfLOS - 1);
      }
      break;

    case '/': {
      SOLDIERTYPE *const sel = GetSelectedMan();
      if (sel) LocateSoldier(sel, 10);
      break;
    }

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      ChangeCurrentSquad(key == '0' ? 9 : key - '1');
      break;

    case '=':
      // if the display cover or line of sight is being displayed
      if (IsKeyDown(SDLK_END) || IsKeyDown(SDLK_DELETE)) {
        if (IsKeyDown(SDLK_DELETE))
          ChangeSizeOfDisplayCover(gGameSettings.ubSizeOfDisplayCover + 1);
        if (IsKeyDown(SDLK_END)) ChangeSizeOfLOS(gGameSettings.ubSizeOfLOS + 1);
      } else if (!(gTacticalStatus.uiFlags & INCOMBAT)) {  // ATE: This key will select everybody in
                                                           // the sector
        FOR_EACH_IN_TEAM(s, OUR_TEAM) {
          if (!OkControllableMerc(s)) continue;
          if (s->uiStatusFlags & (SOLDIER_VEHICLE | SOLDIER_PASSENGER | SOLDIER_DRIVER)) continue;
          s->uiStatusFlags |= SOLDIER_MULTI_SELECTED;
        }
        EndMultiSoldierSelection(TRUE);
      }
      break;

    case '`':
      ToggleTacticalPanels();
      break;

    case 'a':
      BeginAutoBandage();
      break;

    case 'b':
      // Nothing in hand and either not in SM panel, or the matching button is
      // enabled if we are in SM panel
      if (!gpItemPointer &&
          (gsCurInterfacePanel != SM_PANEL || iSMPanelButtons[BURSTMODE_BUTTON]->Enabled())) {
        SetBurstMode();
      }
      break;

    case 'c':
      HandleStanceChangeFromUIKeys(ANIM_CROUCH);
      break;

    case 'd':
      // End turn only if in combat and it is the player's turn
      if (gTacticalStatus.uiFlags & TURNBASED && gTacticalStatus.uiFlags & INCOMBAT &&
          gTacticalStatus.ubCurrentTeam == OUR_TEAM &&
          /* Nothing in hand and the Done button for whichever panel we're in must
             be enabled */
          !gpItemPointer && !gfDisableTacticalPanelButtons &&
          ((gsCurInterfacePanel == SM_PANEL && iSMPanelButtons[SM_DONE_BUTTON]->Enabled()) ||
           (gsCurInterfacePanel == TEAM_PANEL && iTEAMPanelButtons[TEAM_DONE_BUTTON]->Enabled()))) {
        *new_event = I_ENDTURN;
      }
      break;

    case 'e':
      if (SOLDIERTYPE *const sel = GetSelectedMan()) {
        if (sel->bOppCnt > 0) {
          CycleVisibleEnemies(sel);
        } else {
          ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NO_ENEMIES_IN_SIGHT_STR]);
        }
      }
      break;

    case 'f':
      // If there is a selected soldier, and the cursor location is valid
      if (SOLDIERTYPE const *const sel = GetSelectedMan()) {
        GridNo const grid_no = gUIFullTarget ? gUIFullTarget->sGridNo : GetMouseMapPos();
        DisplayRangeToTarget(sel, grid_no);
      }
      break;

    case 'g':
      HandlePlayerTogglingLightEffects(TRUE);
      break;
    case 'h':
      ShouldTheHelpScreenComeUp(HELP_SCREEN_TACTICAL, TRUE);
      break;
    case 'i':
      ToggleItemGlow(!gGameSettings.fOptions[TOPTION_GLOW_ITEMS]);
      break;
    case 'k':
      BeginKeyPanelFromKeyShortcut();
      break;

    case 'l':
      // Nothing in hand and either not in SM panel, or the matching button is
      // enabled if we are in SM panel
      if (!gpItemPointer &&
          (gsCurInterfacePanel != SM_PANEL || iSMPanelButtons[LOOK_BUTTON]->Enabled())) {
        *new_event = LC_CHANGE_TO_LOOK;
      }
      break;

    case 'm':
      // Nothing in hand and the Map Screen button for whichever panel we're in
      // must be enabled
      if (!gpItemPointer && !gfDisableTacticalPanelButtons &&
          !(gTacticalStatus.uiFlags & ENGAGED_IN_CONV) &&
          ((gsCurInterfacePanel == SM_PANEL && iSMPanelButtons[SM_MAP_SCREEN_BUTTON]->Enabled()) ||
           (gsCurInterfacePanel == TEAM_PANEL &&
            iTEAMPanelButtons[TEAM_MAP_SCREEN_BUTTON]->Enabled()))) {
        GoToMapScreenFromTactical();
      }
      break;

    case 'n': {
      GridNo const map_pos = GetMouseMapPos();
      if (!CycleSoldierFindStack(map_pos))  // Are we over a merc stack?
      {
        CycleIntTileFindStack(map_pos);  // If not, now check if we are over a struct stack
      }
      break;
    }

    case 'o':
      // nothing in hand and the Options Screen button for whichever panel we're
      // in must be enabled
      if (!gpItemPointer && !gfDisableTacticalPanelButtons && !fDisableMapInterfaceDueToBattle &&
          (gsCurInterfacePanel != SM_PANEL ||
           iSMPanelButtons[OPTIONS_BUTTON]->Enabled())) {  // Go to Options screen
        guiPreviousOptionScreen = GAME_SCREEN;
        LeaveTacticalScreen(OPTIONS_SCREEN);
      }
      break;

    case 'p':
      HandleStanceChangeFromUIKeys(ANIM_PRONE);
      break;

    case 'r': {
      SOLDIERTYPE *const sel = GetSelectedMan();
      if (sel && !MercInWater(sel) &&
          !(sel->uiStatusFlags & SOLDIER_ROBOT)) {  // Change selected merc to run
        if (sel->usUIMovementMode != WALKING && sel->usUIMovementMode != RUNNING) {
          UIHandleSoldierStanceChange(sel, ANIM_STAND);
        } else {
          sel->usUIMovementMode = RUNNING;
          gfPlotNewMovement = TRUE;
        }
        sel->fUIMovementFast = 1;
      }
      break;
    }

    case 's':
      if (GetSelectedMan()) {
        gfPlotNewMovement = TRUE;
        HandleStanceChangeFromUIKeys(ANIM_STAND);
      }
      break;

    case 't':
      ToggleTreeTops();
      break;
    case 'u':
      if (GetSelectedMan()) *new_event = M_CHANGE_TO_ACTION;
      break;
    case 'v':
      DisplayGameSettings();
      break;
    case 'w':
      ToggleWireFrame();
      break;

    case 'x': {  // Exchange places
      SOLDIERTYPE *const s1 = GetSelectedMan();
      SOLDIERTYPE *const s2 = gUIFullTarget;
      if (s1 && s2 && s2 != s1 && s1->bLife >= OKLIFE &&  // Check if both OK
          s2->bLife >= OKLIFE &&
          CanSoldierReachGridNoInGivenTileLimit(s1, s2->sGridNo, 1,
                                                gsInterfaceLevel) &&
          (s2->bNeutral || s2->bSide == OUR_TEAM) &&  // Exclude enemies
          CanExchangePlaces(s1, s2, TRUE)) {
        SwapMercPositions(*s1, *s2);
        DeductPoints(s1, AP_EXCHANGE_PLACES, 0);
        DeductPoints(s2, AP_EXCHANGE_PLACES, 0);
      }
      break;
    }

    case 'y':
      if (INFORMATION_CHEAT_LEVEL()) *new_event = I_LOSDEBUG;
      break;
    case 'z':
      if (!gpItemPointer) HandleStealthChangeFromUIKeys();
      break;

    case SDLK_INSERT:
      GoIntoOverheadMap();
      break;

    case SDLK_HOME: {
      BOOLEAN &cursor3d = gGameSettings.fOptions[TOPTION_3D_CURSOR];
      cursor3d = !cursor3d;
      wchar_t const *const msg =
          cursor3d ? pMessageStrings[MSG_3DCURSOR_ON] : pMessageStrings[MSG_3DCURSOR_OFF];
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, msg);
      break;
    }

    case SDLK_END: {
      SOLDIERTYPE *const sel = GetSelectedMan();
      if (sel && CheckForMercContMove(sel)) {
        ContinueMercMovement(sel);
        ErasePath();
      }
      break;
    }

    case SDLK_PAGEUP:
      if (guiCurrentScreen != DEBUG_SCREEN) {
        SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel && !gpItemPointer) GotoHigherStance(sel);
      }
      break;

    case SDLK_PAGEDOWN:
      if (guiCurrentScreen != DEBUG_SCREEN) {
        SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel && !gpItemPointer) GotoLowerStance(sel);
      }
      break;

    case SDLK_F1:
    case SDLK_F2:
    case SDLK_F3:
    case SDLK_F4:
    case SDLK_F5:
    case SDLK_F6: {
      uint32_t const idx = key - SDLK_F1;
      HandleSelectMercSlot(idx, true);
      break;
    }

    case SDLK_F11:
      if (DEBUG_CHEAT_LEVEL()) {
        gsQdsEnteringGridNo = GetMouseMapPos();
        LeaveTacticalScreen(QUEST_DEBUG_SCREEN);
      }
      break;

    case SDLK_F12:
      ClearDisplayedListOfTacticalStrings();
      break;
  }
}

static void HandleModShift(uint32_t const key, UIEventKind *const new_event) {
  switch (key) {
    case SDLK_SPACE:
      // Nothing in hand and either not in SM panel, or the matching button is
      // enabled if we are in SM panel
      if (!(gTacticalStatus.uiFlags & ENGAGED_IN_CONV) &&
          (gsCurInterfacePanel != SM_PANEL || iSMPanelButtons[NEXTMERC_BUTTON]->Enabled()) &&
          !InKeyRingPopup()) {
        if (SOLDIERTYPE *const sel = GetSelectedMan()) {  // Only allow if nothing in hand and if in
                                                          // SM panel, the Change Squad button must
                                                          // be enabled
          if (gsCurInterfacePanel != TEAM_PANEL ||
              iTEAMPanelButtons[CHANGE_SQUAD_BUTTON]->Enabled()) {  // Select next squad
            int32_t const current_squad = CurrentSquad();
            SOLDIERTYPE *const new_soldier = FindNextActiveSquad(sel);
            if (new_soldier->bAssignment != current_squad) {
              HandleLocateSelectMerc(new_soldier, true);
              ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_SQUAD_ACTIVE],
                        CurrentSquad() + 1);
              // Center to guy
              LocateSoldier(GetSelectedMan(), SETLOCATOR);
            }
          }
        }

        *new_event = M_ON_TERRAIN;
      }
      break;

    case SDLK_F1:
    case SDLK_F2:
    case SDLK_F3:
    case SDLK_F4:
    case SDLK_F5:
    case SDLK_F6: {
      uint32_t const idx = key - SDLK_F1;
      HandleSelectMercSlot(idx, false);
      break;
    }
  }
}

static void HandleModCtrl(uint32_t const key, UIEventKind *const new_event) {
  switch (key) {
    case 'c':
      if (CHEATER_CHEAT_LEVEL()) ToggleCliffDebug();
      break;

    case 'e':
      if (GameState::getInstance()->isEditorMode()) {
        ToggleMapEdgepoints();
      }
      break;

    case 'f':
      if (INFORMATION_CHEAT_LEVEL()) {  // Toggle frame rate display
        gbFPSDisplay = !gbFPSDisplay;
        EnableFPSOverlay(gbFPSDisplay);
        if (!gbFPSDisplay) SetRenderFlags(RENDER_FLAG_FULL);
      }
      break;

    case 'h':
      if (CHEATER_CHEAT_LEVEL()) *new_event = I_TESTHIT;
      break;
    case 'k':
      if (CHEATER_CHEAT_LEVEL()) GrenadeTest2();
      break;

    case 'l':
      if (!(gTacticalStatus.uiFlags & ENGAGED_IN_CONV)) {
        gfSaveGame = FALSE;
        gfCameDirectlyFromGame = TRUE;
        guiPreviousOptionScreen = GAME_SCREEN;
        LeaveTacticalScreen(SAVE_LOAD_SCREEN);
      }
      break;

    case 'o':
      if (CHEATER_CHEAT_LEVEL()) CreatePlayerControlledMonster();
      break;

    case 's':
      if (!fDisableMapInterfaceDueToBattle && !(gTacticalStatus.uiFlags & ENGAGED_IN_CONV)) {
        if (CanGameBeSaved()) {
          gfSaveGame = TRUE;
          gfCameDirectlyFromGame = TRUE;
          guiPreviousOptionScreen = GAME_SCREEN;
          LeaveTacticalScreen(SAVE_LOAD_SCREEN);
        } else {  // Display a message saying the player cannot save now
          DoMessageBox(MSG_BOX_BASIC_STYLE, zNewTacticalMessages[TCTL_MSG__IRON_MAN_CANT_SAVE_NOW],
                       GAME_SCREEN, MSG_BOX_FLAG_OK, 0, 0);
        }
      }
      break;

    case 't':
      if (CHEATER_CHEAT_LEVEL()) TestCapture();
      break;

    case 'u':
      if (CHEATER_CHEAT_LEVEL() && GetSelectedMan()) {
        FOR_EACH_IN_TEAM(s, OUR_TEAM) {
          if (s->bLife <= 0) continue;

          s->bBreath = s->bBreathMax;  // Get breath back
          s->bLife = s->bLifeMax;      // Get life back
          s->bBleeding = 0;

          fInterfacePanelDirty = DIRTYLEVEL2;
        }
      }
      break;

    case 'w':
      if (CHEATER_CHEAT_LEVEL()) {
        SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel) CreateItem(FLAMETHROWER, 100, &sel->inv[HANDPOS]);
      }
      break;

    case 'z':
      if (INFORMATION_CHEAT_LEVEL()) ToggleZBuffer();
      break;

    case SDLK_PAGEUP:
      // Try to go up towards ground level
      if (CHEATER_CHEAT_LEVEL()) AttemptToChangeFloorLevel(-1);
      break;

    case SDLK_PAGEDOWN:
      // Try to enter a lower underground level
      if (CHEATER_CHEAT_LEVEL()) AttemptToChangeFloorLevel(+1);
      break;

    case SDLK_F12:
      ClearTacticalMessageQueue();
      break;
  }
}

static void HandleModAlt(uint32_t const key, UIEventKind *const new_event) {
  switch (key) {
    case '-': {
      uint32_t const vol = MusicGetVolume();
      MusicSetVolume(vol > 20 ? vol - 20 : 0);
      break;
    }
    case '2':
      if (CHEATER_CHEAT_LEVEL()) ChangeSoldiersBodyType(INFANT_MONSTER, TRUE);
      break;
    case '3':
      if (CHEATER_CHEAT_LEVEL()) EVENT_InitNewSoldierAnim(GetSelectedMan(), KID_SKIPPING, 0, TRUE);
      break;
    case '4':
      if (CHEATER_CHEAT_LEVEL()) ChangeSoldiersBodyType(CRIPPLECIV, TRUE);
      break;
    case '5':
      if (CHEATER_CHEAT_LEVEL()) ChangeSoldiersBodyType(YAM_MONSTER, TRUE);
      break;

    case 'b':
      if (CHEATER_CHEAT_LEVEL()) *new_event = I_NEW_BADMERC;
      break;
    case 'c':
      if (CHEATER_CHEAT_LEVEL()) CreateNextCivType();
      break;

    case 'd':
      if (CHEATER_CHEAT_LEVEL() && gTacticalStatus.uiFlags & TURNBASED &&
          gTacticalStatus.uiFlags & INCOMBAT && gTacticalStatus.ubCurrentTeam == OUR_TEAM &&
          /* Nothing in hand and the Done button for whichever panel we're in must
             be enabled */
          !gpItemPointer && !gfDisableTacticalPanelButtons &&
          ((gsCurInterfacePanel == SM_PANEL && iSMPanelButtons[SM_DONE_BUTTON]->Enabled()) ||
           (gsCurInterfacePanel == TEAM_PANEL && iTEAMPanelButtons[TEAM_DONE_BUTTON]->Enabled()))) {
        FOR_EACH_IN_TEAM(s, OUR_TEAM) {
          if (s->bLife <= 0) continue;
          // Get APs back
          CalcNewActionPoints(s);
          fInterfacePanelDirty = DIRTYLEVEL2;
        }
      }
      break;

    case 'e':
      if (CHEATER_CHEAT_LEVEL()) {
        ToggleViewAllMercs();
        ToggleViewAllItems();
      }
      break;

    case 'f': {
      BOOLEAN &tracking = gGameSettings.fOptions[TOPTION_TRACKING_MODE];
      tracking = !tracking;
      wchar_t const *const msg =
          tracking ? pMessageStrings[MSG_TACKING_MODE_ON] : pMessageStrings[MSG_TACKING_MODE_OFF];
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, msg);
      break;
    }

    case 'g':
      if (CHEATER_CHEAT_LEVEL()) *new_event = I_NEW_MERC;
      break;
    case 'h':
      if (CHEATER_CHEAT_LEVEL()) gfReportHitChances = !gfReportHitChances;
      break;
    case 'i':
      if (CHEATER_CHEAT_LEVEL()) CreateRandomItem();
      break;
    case 'j':
      if (CHEATER_CHEAT_LEVEL()) gfNextFireJam = TRUE;
      break;
    case 'k':
      if (CHEATER_CHEAT_LEVEL()) GrenadeTest1();
      break;

    case 'l':
      if (!(gTacticalStatus.uiFlags & ENGAGED_IN_CONV)) {
        LeaveTacticalScreen(GAME_SCREEN);
        DoQuickLoad();
      }
      break;

    case 'm':
      if (INFORMATION_CHEAT_LEVEL()) {
        *new_event = I_LEVELNODEDEBUG;
        CountLevelNodes();
      }
      break;

    case 'n':
      if (INFORMATION_CHEAT_LEVEL() && gUIFullTarget) {
        static uint16_t gQuoteNum = 0;
        TacticalCharacterDialogue(gUIFullTarget, gQuoteNum++);
      }
      break;

    case 'o':
      if (CHEATER_CHEAT_LEVEL()) {
        gStrategicStatus.usPlayerKills +=
            NumEnemiesInAnySector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
        ObliterateSector();
      }
      break;

    case 'r':
      if (CHEATER_CHEAT_LEVEL()) {  // Reload selected merc's weapon
        SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel) ReloadWeapon(sel, sel->ubAttackingHand);
      }
      break;

    case 's':
      if (!fDisableMapInterfaceDueToBattle && !(gTacticalStatus.uiFlags & ENGAGED_IN_CONV)) {
        if (CanGameBeSaved()) {
          guiPreviousOptionScreen = GAME_SCREEN;
          DoQuickSave();
        } else {  // Display a message saying the player cannot save now
          DoMessageBox(MSG_BOX_BASIC_STYLE, zNewTacticalMessages[TCTL_MSG__IRON_MAN_CANT_SAVE_NOW],
                       GAME_SCREEN, MSG_BOX_FLAG_OK, 0, 0);
        }
      }
      break;

    case 't':
      if (CHEATER_CHEAT_LEVEL()) TeleportSelectedSoldier();
      break;
    case 'u':
      if (CHEATER_CHEAT_LEVEL()) RefreshSoldier();
      break;

    case 'w':
      if (CHEATER_CHEAT_LEVEL()) {
        if (InItemDescriptionBox()) {  // Swap item in description panel
          CycleItemDescriptionItem();
        } else {
          CycleSelectedMercsItem();
        }
      }
      break;

    case 'y':
      if (CHEATER_CHEAT_LEVEL()) {
        QuickCreateProfileMerc(CIV_TEAM, MARIA);
        RecruitEPC(MARIA);
      }

      if (SOLDIERTYPE *const robot = FindSoldierByProfileID(ROBOT)) {
        OBJECTTYPE o;
        CreateItem(G41, 100, &o);
        AutoPlaceObject(robot, &o, FALSE);
      }
      break;

    case 'z':  // Toggle squad's stealth mode
    {
      // Check if at least one guy is on stealth
      bool stealth_on = true;
      CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
        if (!OkControllableMerc(s)) continue;
        if (s->bAssignment != CurrentSquad()) continue;
        if (!s->bStealthMode) continue;
        stealth_on = false;
        break;
      }

      FOR_EACH_IN_TEAM(s, OUR_TEAM) {
        if (!OkControllableMerc(s)) continue;
        if (s->bAssignment != CurrentSquad()) continue;
        if (AM_A_ROBOT(s)) continue;

        if (gpSMCurrentMerc == s) gfUIStanceDifferent = TRUE;
        s->bStealthMode = stealth_on;
      }

      fInterfacePanelDirty = DIRTYLEVEL2;

      // Display message
      wchar_t const *const msg = stealth_on ? pMessageStrings[MSG_SQUAD_ON_STEALTHMODE]
                                            : pMessageStrings[MSG_SQUAD_OFF_STEALTHMODE];
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, msg);
      break;
    }
  }
}

void GetKeyboardInput(UIEventKind *const puiNewEvent) {
  InputAtom InputEvent;
  BOOLEAN fKeyTaken = FALSE;

  SGPPoint MousePos;
  GetMousePos(&MousePos);

  while (DequeueEvent(&InputEvent)) {
    MouseSystemHook(InputEvent.usEvent, MousePos.iX, MousePos.iY);

    // handle for fast help text for interface stuff
    if (IsTheInterfaceFastHelpTextActive()) {
      if (InputEvent.usEvent == KEY_UP) {
        ShutDownUserDefineHelpTextRegions();
      }

      continue;
    }

    // Check for waiting for keyboard advance
    if (gfUIWaitingForUserSpeechAdvance && InputEvent.usEvent == KEY_UP) {
      // We have a key, advance!
      DialogueAdvanceSpeech();

      // Ignore anything else
      continue;
    }

    // ATE: if game paused because fo player, unpasue with any key
    if (gfPauseDueToPlayerGamePause && InputEvent.usEvent == KEY_UP) {
      HandlePlayerPauseUnPauseOfGame();

      continue;
    }

    if (InputEvent.usEvent == KEY_DOWN) {
      if (g_ui_message_overlay != NULL) {
        EndUIMessage();
        continue;
      }

      // End auto bandage if we want....
      if (gTacticalStatus.fAutoBandageMode) {
        AutoBandage(FALSE);
        *puiNewEvent = LU_ENDUILOCK;
      }
    }

    if (gUIKeyboardHook != NULL) {
      fKeyTaken = gUIKeyboardHook(&InputEvent);
    }
    if (fKeyTaken) {
      continue;
    }

    /*
    if( (InputEvent.usEvent == KEY_DOWN )&& ( InputEvent.usParam == ) )
    {
            HandlePlayerPauseUnPauseOfGame( );
    }
    */

    if (InputEvent.usEvent == KEY_UP && InputEvent.usParam == SDLK_PAUSE &&
        !(gTacticalStatus.uiFlags & ENGAGED_IN_CONV)) {
      // Pause game!
      HandlePlayerPauseUnPauseOfGame();
    }

    // FIRST DO KEYS THAT ARE USED EVERYWHERE!
    if ((InputEvent.usEvent == KEY_DOWN) && (InputEvent.usParam == 'x') &&
        (InputEvent.usKeyState & ALT_DOWN)) {
      HandleShortCutExitState();
    }

    if (InputEvent.usEvent == KEY_UP && InputEvent.usParam == SDLK_ESCAPE) {
      if (AreInMeanwhile() && gCurrentMeanwhileDef.ubMeanwhileID != INTERROGATION) {
        DeleteTalkingMenu();
        EndMeanwhile();
      }
    }

    // Break of out IN CONV...
    if (CHEATER_CHEAT_LEVEL()) {
      if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_RETURN &&
          InputEvent.usKeyState & ALT_DOWN) {
        if (gTacticalStatus.uiFlags & ENGAGED_IN_CONV) {
          gTacticalStatus.uiFlags &= (~ENGAGED_IN_CONV);
          giNPCReferenceCount = 0;
        }
      }
    }

    if (gTacticalStatus.uiFlags & TURNBASED && (gTacticalStatus.uiFlags & INCOMBAT)) {
      {
        if (gTacticalStatus.ubCurrentTeam != OUR_TEAM) {
          if (CHEATER_CHEAT_LEVEL()) {
            if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_RETURN &&
                InputEvent.usKeyState & ALT_DOWN) {
              // ESCAPE ENEMY'S TURN
              EndAIDeadlock();

              // Decrease global busy  counter...
              gTacticalStatus.ubAttackBusyCount = 0;

              guiPendingOverrideEvent = LU_ENDUILOCK;
              UIHandleLUIEndLock(NULL);
            }
            if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_RETURN &&
                InputEvent.usKeyState & CTRL_DOWN) {
              EscapeUILock();
            }
          }
        } else {
          if (CHEATER_CHEAT_LEVEL() && InputEvent.usEvent == KEY_DOWN &&
              InputEvent.usParam == SDLK_RETURN && InputEvent.usKeyState & CTRL_DOWN) {
            // UNLOCK UI
            EscapeUILock();
          } else if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_RETURN) {
            // Cycle through enemys
            CycleThroughKnownEnemies();
          }
        }
      }
    }

    if (gfInTalkPanel) {
      HandleTalkingMenuKeys(&InputEvent, puiNewEvent);
    }

    // Do some checks based on what mode we are in
    switch (gCurrentUIMode) {
      case EXITSECTORMENU_MODE:

        HandleSectorExitMenuKeys(&InputEvent, puiNewEvent);
        continue;

      case GETTINGITEM_MODE:

        HandleItemMenuKeys(&InputEvent, puiNewEvent);
        continue;

      case MENU_MODE:

        HandleMenuKeys(&InputEvent, puiNewEvent);
        continue;

      case OPENDOOR_MENU_MODE:

        HandleOpenDoorMenuKeys(&InputEvent, puiNewEvent);
        continue;
      default:
        break;
    }

    // CHECK ESC KEYS HERE....
    if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_ESCAPE) {
      // EscapeUILock( );

      // Cancel out of spread burst...
      gfBeginBurstSpreadTracking = FALSE;
      gfRTClickLeftHoldIntercepted = TRUE;
      SOLDIERTYPE *const sel = GetSelectedMan();
      if (sel != NULL) sel->fDoSpread = FALSE;

      // Befone anything, delete popup box!
      EndUIMessage();

      if (InItemDescriptionBox()) {
        DeleteItemDescriptionBox();
      } else if (InKeyRingPopup()) {
        DeleteKeyRingPopup();
      }

      if (gCurrentUIMode == MENU_MODE) {
        // If we get a hit here and we're in menu mode, quit the menu mode
        EndMenuEvent(guiCurrentEvent);
      }

      if (gCurrentUIMode == HANDCURSOR_MODE) {
        *puiNewEvent = A_CHANGE_TO_MOVE;
      }

      if (!(gTacticalStatus.uiFlags & ENGAGED_IN_CONV)) {
        if (sel != NULL) {
          // If soldier is not stationary, stop
          StopSoldier(sel);
          *puiNewEvent = A_CHANGE_TO_MOVE;
        }
        // ATE: OK, stop any mercs who are moving by selection method....
        StopRubberBandedMercFromMoving();
      }
    }

    // CHECK ESC KEYS HERE....
    if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_BACKSPACE) {
      StopAnyCurrentlyTalkingSpeech();
    }

    // IF UI HAS LOCKED, ONLY ALLOW EXIT!
    if (gfDisableRegionActive || gfUserTurnRegionActive) {
      continue;
    }

    // Check all those we want if enemy's turn
    if ((InputEvent.usEvent == KEY_UP) && (InputEvent.usParam == 'q')) {
      if (InputEvent.usKeyState & ALT_DOWN) {
        if (CHEATER_CHEAT_LEVEL()) {
          static BOOLEAN fShowRoofs = TRUE;
          int32_t x;
          uint16_t usType;

          // Toggle removal of roofs...
          fShowRoofs = !fShowRoofs;

          for (x = 0; x < WORLD_MAX; x++) {
            for (usType = FIRSTROOF; usType <= LASTSLANTROOF; usType++) {
              HideStructOfGivenType(x, usType, !fShowRoofs);
            }
          }
          InvalidateWorldRedundency();
          SetRenderFlags(RENDER_FLAG_FULL);
        }

      } else if (InputEvent.usKeyState & CTRL_DOWN) {
      } else {
        if (INFORMATION_CHEAT_LEVEL()) {
          *puiNewEvent = I_SOLDIERDEBUG;
        }
      }
    }

    if (InputEvent.usEvent == KEY_DOWN) {
      uint16_t const mod = InputEvent.usKeyState;
      uint32_t const key = InputEvent.usParam;

      if (mod == CTRL_DOWN) {
        if (gubCheatLevel < strlen(getCheatCode())) {
          if (key == getCheatCode()[gubCheatLevel]) {
            if (++gubCheatLevel == strlen(getCheatCode())) {
              ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_CHEAT_LEVEL_TWO]);
              AddHistoryToPlayersLog(HISTORY_CHEAT_ENABLED, 0, GetWorldTotalMin(), -1, -1);
            }
            continue;
          }
        } else if (gubCheatLevel == strlen(getCheatCode()) && key == 'b') {
          ++gubCheatLevel;
          continue;
        }
      }

      if (gubCheatLevel < strlen(getCheatCode())) RESET_CHEAT_LEVEL();

      switch (mod) {
        case 0:
          HandleModNone(key, puiNewEvent);
          break;
        case SHIFT_DOWN:
          HandleModShift(key, puiNewEvent);
          break;
        case CTRL_DOWN:
          HandleModCtrl(key, puiNewEvent);
          break;
        case ALT_DOWN:
          HandleModAlt(key, puiNewEvent);
          break;

        case CTRL_DOWN | ALT_DOWN:
          if (key == 'k') {
            if (CHEATER_CHEAT_LEVEL()) {  // Next shot by anybody is auto kill
              gfNextShotKills = !gfNextShotKills;
            }
          }
          break;
      }
    }
  }
}

static void HandleTalkingMenuKeys(InputAtom *const pInputEvent, UIEventKind *const puiNewEvent) {
  // CHECK ESC KEYS HERE....
  if (pInputEvent->usEvent == KEY_UP) {
    if (pInputEvent->usParam == SDLK_ESCAPE) {
      // Handle esc in talking menu
      if (HandleTalkingMenuEscape(TRUE, TRUE)) {
        *puiNewEvent = A_CHANGE_TO_MOVE;
      }
    } else if (pInputEvent->usParam == SDLK_BACKSPACE) {
      HandleTalkingMenuBackspace();
    }
  }
}

static void HandleSectorExitMenuKeys(InputAtom *const pInputEvent, UIEventKind *const puiNewEvent) {
  // CHECK ESC KEYS HERE....
  if (pInputEvent->usEvent == KEY_UP && pInputEvent->usParam == SDLK_ESCAPE) {
    // Handle esc in talking menu
    RemoveSectorExitMenu(FALSE);

    *puiNewEvent = A_CHANGE_TO_MOVE;
  }
}

static void HandleOpenDoorMenuKeys(InputAtom *const pInputEvent, UIEventKind *const puiNewEvent) {
  // CHECK ESC KEYS HERE....
  if (pInputEvent->usEvent == KEY_UP && pInputEvent->usParam == SDLK_ESCAPE) {
    // Handle esc in talking menu
    CancelOpenDoorMenu();
    HandleOpenDoorMenu();
    *puiNewEvent = A_CHANGE_TO_MOVE;
  }
}

static void HandleMenuKeys(InputAtom *const pInputEvent, UIEventKind *const puiNewEvent) {
  // CHECK ESC KEYS HERE....
  if (pInputEvent->usEvent == KEY_UP && pInputEvent->usParam == SDLK_ESCAPE) {
    // Handle esc in talking menu
    CancelMovementMenu();

    *puiNewEvent = A_CHANGE_TO_MOVE;
  }
}

static void HandleItemMenuKeys(InputAtom *const pInputEvent, UIEventKind *const puiNewEvent) {
  // CHECK ESC KEYS HERE....
  if (pInputEvent->usEvent == KEY_UP && pInputEvent->usParam == SDLK_ESCAPE) {
    // Handle esc in talking menu
    RemoveItemPickupMenu();
    *puiNewEvent = A_CHANGE_TO_MOVE;
  }
}

BOOLEAN HandleCheckForExitArrowsInput(BOOLEAN fAdjustConfirm) {
  // If not in move mode, return!
  if (gCurrentUIMode != MOVE_MODE) {
    return (FALSE);
  }

  const SOLDIERTYPE *const sel = GetSelectedMan();
  if (sel == NULL) return FALSE;

  // ATE: Remove confirm for exit arrows...
  fAdjustConfirm = TRUE;
  gfUIConfirmExitArrows = TRUE;

  // Return right away, saying that we are in this mode, don't do any normal
  // stuff!
  if (guiCurrentUICursor == NOEXIT_EAST_UICURSOR || guiCurrentUICursor == NOEXIT_WEST_UICURSOR ||
      guiCurrentUICursor == NOEXIT_NORTH_UICURSOR || guiCurrentUICursor == NOEXIT_SOUTH_UICURSOR ||
      guiCurrentUICursor == NOEXIT_GRID_UICURSOR) {
    // Yeah, but add a message....
    if (gfInvalidTraversal) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK,
                TacticalStr[CANNOT_LEAVE_SECTOR_FROM_SIDE_STR]);
      gfInvalidTraversal = FALSE;
    } else if (gfRobotWithoutControllerAttemptingTraversal) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, gzLateLocalizedString[STR_LATE_01]);
      gfRobotWithoutControllerAttemptingTraversal = FALSE;
    } else if (gfLoneEPCAttemptingTraversal) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK,
                pExitingSectorHelpText[EXIT_GUI_ESCORTED_CHARACTERS_CANT_LEAVE_SECTOR_ALONE_STR],
                sel->name);
      gfLoneEPCAttemptingTraversal = FALSE;
    } else if (gubLoneMercAttemptingToAbandonEPCs) {
      wchar_t str[256];
      if (gubLoneMercAttemptingToAbandonEPCs == 1) {       // Use the singular version of the string
        if (gMercProfiles[sel->ubProfile].bSex == MALE) {  // male singular
          swprintf(str, lengthof(str),
                   pExitingSectorHelpText[EXIT_GUI_MERC_CANT_ISOLATE_EPC_HELPTEXT_MALE_SINGULAR],
                   sel->name, gPotentiallyAbandonedEPC->name);
        } else {  // female singular
          swprintf(str, lengthof(str),
                   pExitingSectorHelpText[EXIT_GUI_MERC_CANT_ISOLATE_EPC_HELPTEXT_FEMALE_SINGULAR],
                   sel->name, gPotentiallyAbandonedEPC->name);
        }
      } else {                                             // Use the plural version of the string
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
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, str);
      gubLoneMercAttemptingToAbandonEPCs = FALSE;
    } else {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[MERC_IS_TOO_FAR_AWAY_STR],
                sel->name);
    }

    return (TRUE);
  }

  // Check if we want to exit!
  if (guiCurrentUICursor == EXIT_GRID_UICURSOR ||
      guiCurrentUICursor == CONFIRM_EXIT_GRID_UICURSOR) {
    if (fAdjustConfirm) {
      if (!gfUIConfirmExitArrows) {
        gfUIConfirmExitArrows = TRUE;
      } else {
        const GridNo sMapPos = GetMouseMapPos();
        if (sMapPos == NOWHERE) return FALSE;

        // Goto next sector
        // SimulateMouseMovement( gusMouseXPos - 5, gusMouseYPos );
        InitSectorExitMenu(DIRECTION_EXITGRID, sMapPos);
      }
    }
    return (TRUE);
  }

  // Check if we want to exit!
  if (guiCurrentUICursor == EXIT_EAST_UICURSOR ||
      guiCurrentUICursor == CONFIRM_EXIT_EAST_UICURSOR) {
    if (fAdjustConfirm) {
      if (!gfUIConfirmExitArrows) {
        gfUIConfirmExitArrows = TRUE;
      } else {
        // Goto next sector
        // SimulateMouseMovement( gusMouseXPos - 5, gusMouseYPos );
        InitSectorExitMenu(EAST, 0);
      }
    }
    return (TRUE);
  }
  if (guiCurrentUICursor == EXIT_WEST_UICURSOR ||
      guiCurrentUICursor == CONFIRM_EXIT_WEST_UICURSOR) {
    if (fAdjustConfirm) {
      if (!gfUIConfirmExitArrows) {
        gfUIConfirmExitArrows = TRUE;
      } else {
        // Goto next sector
        // SimulateMouseMovement( gusMouseXPos + 5, gusMouseYPos );
        InitSectorExitMenu(WEST, 0);
      }
    }
    return (TRUE);
  }
  if (guiCurrentUICursor == EXIT_NORTH_UICURSOR ||
      guiCurrentUICursor == CONFIRM_EXIT_NORTH_UICURSOR) {
    if (fAdjustConfirm) {
      if (!gfUIConfirmExitArrows) {
        gfUIConfirmExitArrows = TRUE;
      } else {
        // Goto next sector
        // SimulateMouseMovement( gusMouseXPos, gusMouseYPos + 5 );
        InitSectorExitMenu(NORTH, 0);
      }
    }
    return (TRUE);
  }
  if (guiCurrentUICursor == EXIT_SOUTH_UICURSOR ||
      guiCurrentUICursor == CONFIRM_EXIT_SOUTH_UICURSOR) {
    if (fAdjustConfirm) {
      if (!gfUIConfirmExitArrows) {
        gfUIConfirmExitArrows = TRUE;
      } else {
        // Goto next sector
        // SimulateMouseMovement( gusMouseXPos, gusMouseYPos - 5);
        InitSectorExitMenu(SOUTH, 0);
      }
    }
    return (TRUE);
  }
  return (FALSE);
}

// Simple function implementations called by keyboard input

static void CreateRandomItem() {
  OBJECTTYPE Object;
  const GridNo usMapPos = GetMouseMapPos();
  if (usMapPos != NOWHERE) {
    CreateItem((uint16_t)(Random(35) + 1), 100, &Object);
    AddItemToPool(usMapPos, &Object, INVISIBLE, 0, 0, 0);
  }
}

static void ToggleViewAllMercs() {  // Set option to show all mercs
  gTacticalStatus.uiFlags ^= SHOW_ALL_MERCS;
  SetRenderFlags(RENDER_FLAG_FULL);
}

static void ToggleViewAllItems() {
  gTacticalStatus.uiFlags ^= SHOW_ALL_ITEMS;
  ToggleItemGlow(gGameSettings.fOptions[TOPTION_GLOW_ITEMS]);
  SetRenderFlags(RENDER_FLAG_FULL);
}

static void CycleSelectedMercsItem() {
  // Cycle selected guy's item...
  SOLDIERTYPE *const tgt = gUIFullTarget;
  if (tgt != NULL) {
    uint16_t usOldItem = tgt->inv[HANDPOS].usItem;

    usOldItem++;

    if (usOldItem > MAX_WEAPONS) {
      usOldItem = 0;
    }

    CreateItem(usOldItem, 100, &tgt->inv[HANDPOS]);
    DirtyMercPanelInterface(tgt, DIRTYLEVEL2);
  }
}

static void ToggleWireFrame() {
  uint8_t &show_wireframe = gGameSettings.fOptions[TOPTION_TOGGLE_WIREFRAME];
  show_wireframe = !show_wireframe;
  wchar_t const *const msg = show_wireframe ? pMessageStrings[MSG_WIREFRAMES_ADDED]
                                            : pMessageStrings[MSG_WIREFRAMES_REMOVED];
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, msg);
  SetRenderFlags(RENDER_FLAG_FULL);
}

static void RefreshSoldier() {
  // CHECK IF WE'RE ON A GUY ( EITHER SELECTED, OURS, OR THEIRS
  SOLDIERTYPE *const tgt = gUIFullTarget;
  if (tgt != NULL) ReviveSoldier(tgt);
}

static void ChangeSoldiersBodyType(SoldierBodyType const ubBodyType,
                                   BOOLEAN const fCreateNewPalette) {
  SOLDIERTYPE *const sel = GetSelectedMan();
  if (sel == NULL) return;

  sel->ubBodyType = ubBodyType;
  EVENT_InitNewSoldierAnim(sel, STANDING, 0, TRUE);
  if (fCreateNewPalette) {
    CreateSoldierPalettes(*sel);

    switch (ubBodyType) {
      case ADULTFEMALEMONSTER:
      case AM_MONSTER:
      case YAF_MONSTER:
      case YAM_MONSTER:
      case LARVAE_MONSTER:
      case INFANT_MONSTER:
      case QUEENMONSTER:
        sel->uiStatusFlags |= SOLDIER_MONSTER;
        memset(&sel->inv, 0, sizeof(OBJECTTYPE) * NUM_INV_SLOTS);
        AssignCreatureInventory(sel);
        CreateItem(CREATURE_YOUNG_MALE_SPIT, 100, &sel->inv[HANDPOS]);
        break;

      case TANK_NW:
      case TANK_NE:
        sel->uiStatusFlags |= SOLDIER_VEHICLE;
        // sel->inv[HANDPOS].usItem = TANK_CANNON;
        sel->inv[HANDPOS].usItem = MINIMI;
        sel->bVehicleID = AddVehicleToList(sel->sSectorX, sel->sSectorY, sel->bSectorZ, HUMMER);
        break;
      default:
        break;
    }
  }
}

static void TeleportSelectedSoldier() {
  // CHECK IF WE'RE ON A GUY ( EITHER SELECTED, OURS, OR THEIRS
  SOLDIERTYPE *const sel = GetSelectedMan();
  if (sel == NULL) return;

  const GridNo usMapPos = GetMouseMapPos();
  if (usMapPos == NOWHERE) return;

  // Check level first....
  if (gsInterfaceLevel == 0) {
    SetSoldierHeight(sel, 0);
    TeleportSoldier(*sel, usMapPos, false);
    EVENT_StopMerc(sel);
  } else if (FindStructure(usMapPos, STRUCTURE_ROOF) != NULL)  // Is there a roof?
  {
    SetSoldierHeight(sel, 50.0);
    TeleportSoldier(*sel, usMapPos, true);
    EVENT_StopMerc(sel);
  }
}

static void ToggleTreeTops() {
  uint8_t &show_trees = gGameSettings.fOptions[TOPTION_TOGGLE_TREE_TOPS];
  show_trees = !show_trees;
  wchar_t const *const msg =
      show_trees ? TacticalStr[SHOWING_TREETOPS_STR] : TacticalStr[REMOVING_TREETOPS_STR];
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, msg);
  SetTreeTopStateForMap();
}

static void ToggleZBuffer() {
  uint32_t &flags = gTacticalStatus.uiFlags;
  flags ^= SHOW_Z_BUFFER;
  if (!(flags & SHOW_Z_BUFFER)) SetRenderFlags(RENDER_FLAG_FULL);
}

static void SetBurstMode() {
  SOLDIERTYPE *const sel = GetSelectedMan();
  if (sel != NULL) ChangeWeaponMode(sel);
}

static void ObliterateSector() {
  FOR_EACH_NON_PLAYER_SOLDIER(s) {
    if (s->bNeutral || s->bSide == OUR_TEAM) continue;
    EVENT_SoldierGotHit(s, 0, 400, 0, s->bDirection, 320, 0, FIRE_WEAPON_NO_SPECIAL,
                        s->bAimShotLocation, NOWHERE);
  }
}

static void CreateNextCivType() {
  static int8_t bBodyType = FATCIV;

  const GridNo usMapPos = GetMouseMapPos();
  if (usMapPos == NOWHERE) return;

  SOLDIERCREATE_STRUCT MercCreateStruct;
  memset(&MercCreateStruct, 0, sizeof(MercCreateStruct));
  MercCreateStruct.ubProfile = NO_PROFILE;
  MercCreateStruct.sSectorX = gWorldSectorX;
  MercCreateStruct.sSectorY = gWorldSectorY;
  MercCreateStruct.bSectorZ = gbWorldSectorZ;
  MercCreateStruct.bBodyType = bBodyType;
  MercCreateStruct.bDirection = SOUTH;

  if (++bBodyType > KIDCIV) bBodyType = FATCIV;

  MercCreateStruct.bTeam = CIV_TEAM;
  MercCreateStruct.sInsertionGridNo = usMapPos;
  RandomizeNewSoldierStats(&MercCreateStruct);

  SOLDIERTYPE *const s = TacticalCreateSoldier(MercCreateStruct);
  if (s != NULL) {
    AddSoldierToSector(s);

    // So we can see them!
    AllTeamsLookForAll(NO_INTERRUPTS);
  }
}

static void ToggleCliffDebug() {
  wchar_t const *msg;
  gTacticalStatus.uiFlags ^= DEBUGCLIFFS;
  if (gTacticalStatus.uiFlags & DEBUGCLIFFS) {
    msg = L"Cliff debug ON.";
  } else {
    SetRenderFlags(RENDER_FLAG_FULL);
    msg = L"Cliff debug OFF.";
  }
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_TESTVERSION, msg);
}

static void GrenadeTest1() {
  // Get mousexy
  int16_t sX, sY;
  if (GetMouseXY(&sX, &sY)) {
    OBJECTTYPE Object;
    Object.usItem = MUSTARD_GRENADE;
    Object.bStatus[0] = 100;
    Object.ubNumberOfObjects = 1;
    CreatePhysicalObject(&Object, 60, sX * CELL_X_SIZE, sY * CELL_Y_SIZE, 256, -20, 20, 158, NULL,
                         THROW_ARM_ITEM, 0);
  }
}

static void GrenadeTest2() {
  // Get mousexy
  int16_t sX, sY;
  if (GetMouseXY(&sX, &sY)) {
    OBJECTTYPE Object;
    Object.usItem = HAND_GRENADE;
    Object.bStatus[0] = 100;
    Object.ubNumberOfObjects = 1;
    CreatePhysicalObject(&Object, 60, sX * CELL_X_SIZE, sY * CELL_Y_SIZE, 256, 0, -30, 158, NULL,
                         THROW_ARM_ITEM, 0);
  }
}

static void CreatePlayerControlledMonster() {
  const GridNo usMapPos = GetMouseMapPos();
  if (usMapPos == NOWHERE) return;

  SOLDIERCREATE_STRUCT MercCreateStruct;
  memset(&MercCreateStruct, 0, sizeof(MercCreateStruct));
  MercCreateStruct.ubProfile = NO_PROFILE;
  MercCreateStruct.sSectorX = gWorldSectorX;
  MercCreateStruct.sSectorY = gWorldSectorY;
  MercCreateStruct.bSectorZ = gbWorldSectorZ;
  // Note:  only gets called if Alt and/or Ctrl isn't pressed!
  MercCreateStruct.bBodyType = (IsKeyDown(SDLK_INSERT) ? QUEENMONSTER : ADULTFEMALEMONSTER);
  MercCreateStruct.bTeam = CREATURE_TEAM;
  MercCreateStruct.sInsertionGridNo = usMapPos;
  RandomizeNewSoldierStats(&MercCreateStruct);

  SOLDIERTYPE *const s = TacticalCreateSoldier(MercCreateStruct);
  if (s != NULL) AddSoldierToSector(s);
}

static bool CheckForAndHandleHandleVehicleInteractiveClick(SOLDIERTYPE &s,
                                                           BOOLEAN const fMovementMode) {
  SOLDIERTYPE const *const tgt = gUIFullTarget;
  if (!tgt) return false;
  if (!OK_ENTERABLE_VEHICLE(tgt)) return false;
  if (tgt->bVisible == -1) return false;
  if (!OKUseVehicle(tgt->ubProfile)) return false;

  VEHICLETYPE &v = GetVehicle(tgt->bVehicleID);
  if (GetNumberInVehicle(v) != 0 && fMovementMode) return false;

  // Find a gridno closest to sweetspot
  GridNo const action_pos =
      FindGridNoFromSweetSpotWithStructDataFromSoldier(&s, s.usUIMovementMode, 5, 0, tgt);
  if (action_pos == NOWHERE) return false;

  // Calculate AP costs
  int16_t const ap_cost =
      PlotPath(&s, action_pos, NO_COPYROUTE, FALSE, s.usUIMovementMode, s.bActionPoints);
  if (!EnoughPoints(&s, ap_cost, 0, TRUE)) return false;

  DoMercBattleSound(&s, BATTLE_SOUND_OK1);

  // Check if we are at this gridno now
  if (s.sGridNo != action_pos) {
    // Send pending action
    s.ubPendingAction = MERC_ENTER_VEHICLE;
    s.sPendingActionData2 = tgt->sGridNo;
    s.ubPendingActionAnimCount = 0;
    // Walk up to dest first
    EVENT_InternalGetNewSoldierPath(&s, action_pos, s.usUIMovementMode, 3, s.fNoAPToFinishMove);
    SetUIBusy(&s);
  } else {
    PutSoldierInVehicle(s, v);
  }
  return true;
}

void HandleHandCursorClick(uint16_t const map_pos, UIEventKind *const new_event) {
  SOLDIERTYPE *const s = GetSelectedMan();
  if (!s) return;

  // If we are out of breath, no cursor
  if (s->bBreath < OKBREATH && s->bCollapsed) return;

  if (CheckForAndHandleHandleVehicleInteractiveClick(*s, FALSE)) return;

  // Check if we are on a merc... if so.. steal!
  SOLDIERTYPE const *const tgt = gUIFullTarget;
  if (tgt && guiUIFullTargetFlags & ENEMY_MERC && !(guiUIFullTargetFlags & UNCONSCIOUS_MERC)) {
    GridNo adjusted_gridno;
    GridNo action_gridno = FindAdjacentGridEx(s, tgt->sGridNo, 0, &adjusted_gridno, TRUE, FALSE);
    if (action_gridno == -1) action_gridno = adjusted_gridno;

    // Steal!
    int16_t const ap_cost = GetAPsToStealItem(s, action_gridno);
    if (EnoughPoints(s, ap_cost, 0, TRUE)) {
      MercStealFromMerc(s, tgt);
      *new_event = A_CHANGE_TO_MOVE;
    }
    return;
  }

  // Default action gridno to mouse
  GridNo action_gridno = map_pos;

  // If we are over an interactive struct, adjust gridno to this
  GridNo int_tile_gridno;
  STRUCTURE *structure;
  LEVELNODE const *const int_tile =
      ConditionalGetCurInteractiveTileGridNoAndStructure(&int_tile_gridno, &structure, FALSE);
  bool ignore_items = false;
  if (int_tile) {
    action_gridno = int_tile_gridno;

    if ((structure->fFlags & STRUCTURE_SWITCH) ||
        (structure->fFlags & STRUCTURE_ANYDOOR && action_gridno != map_pos)) {
      ignore_items = true;
    }
  }

  // Check if we are over an item pool
  // ATE: Ignore items will be set if over a switch interactive tile
  ITEM_POOL const *const item_pool = ignore_items ? 0 : GetItemPool(action_gridno, s->bLevel);
  if (item_pool && IsItemPoolVisible(item_pool)) {
    if (AM_AN_EPC(s)) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[EPC_CANNOT_DO_THAT]);
    } else if (UIOkForItemPickup(s, action_gridno)) {
      int8_t const zlevel = GetZLevelOfItemPoolGivenStructure(action_gridno, s->bLevel, structure);
      SoldierPickupItem(s, -1, action_gridno, zlevel);
      *new_event = A_CHANGE_TO_MOVE;
    }
  } else if (int_tile && !(structure->fFlags & STRUCTURE_HASITEMONTOP)) {
    GridNo const adjacent_gridno = FindAdjacentGridEx(s, action_gridno, 0, 0, FALSE, TRUE);
    if (adjacent_gridno != -1) action_gridno = adjacent_gridno;

    // If this is not the same tile as ours, check if we can get to dest!
    if (action_gridno != s->sGridNo && gsCurrentActionPoints == 0) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NO_PATH]);
    } else if (SelectedMercCanAffordMove()) {
      *new_event = C_MOVE_MERC;
    }
  } else { /* ATE: Here, the poor player wants to search something that does not
            * exist. Why should we not let them make fools of themselves? */
    if (AM_AN_EPC(s)) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[EPC_CANNOT_DO_THAT]);
    } else if (s->bMorale < 30)  // Check morale, if < threashold, refuse
    {
      TacticalCharacterDialogue(s, QUOTE_REFUSING_ORDER);
    } else if (gsCurrentActionPoints == 0) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NO_PATH]);
    } else {
      SoldierPickupItem(s, -1, action_gridno, 0);
      *new_event = A_CHANGE_TO_MOVE;
    }
  }
}

int8_t HandleMoveModeInteractiveClick(uint16_t const usMapPos) {
  SOLDIERTYPE *const sel = GetSelectedMan();
  if (!sel) return 0;

  // ATE: If we are a vehicle, no moving!
  if (sel->uiStatusFlags & SOLDIER_VEHICLE) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[VEHICLE_CANT_MOVE_IN_TACTICAL]);
    return -3;
  }

  // OK, check for height differences
  if (gpWorldLevelData[usMapPos].sHeight != gpWorldLevelData[sel->sGridNo].sHeight) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[CANT_GET_THERE]);
    return -1;
  }

  // See if we are over a vehicle, and walk up to it and enter
  if (CheckForAndHandleHandleVehicleInteractiveClick(*sel, TRUE)) {
    return -1;
  }

  // Check if we are over a civillian
  SOLDIERTYPE *const tgt = gUIFullTarget;
  if (tgt) {
    if (ValidQuickExchangePosition() && CanExchangePlaces(sel, tgt, TRUE)) {
      SwapMercPositions(*sel, *tgt);
    }
    return -3;
  }

  int16_t sIntTileGridNo;
  STRUCTURE *pStructure;
  LEVELNODE *const pIntTile = GetCurInteractiveTileGridNoAndStructure(&sIntTileGridNo, &pStructure);
  if (!pIntTile) return 0;

  // Check if we are over an item pool, take precedence over that, except for
  // switches
  ITEM_POOL const *const pItemPool = GetItemPool(sIntTileGridNo, sel->bLevel);
  if (pItemPool && !(pStructure->fFlags & (STRUCTURE_SWITCH | STRUCTURE_ANYDOOR))) {
    if (AM_AN_EPC(sel)) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[EPC_CANNOT_DO_THAT]);
    } else if (UIOkForItemPickup(sel, sIntTileGridNo)) {
      int8_t const bZLevel = GetLargestZLevelOfItemPool(pItemPool);
      if (AnyItemsVisibleOnLevel(pItemPool, bZLevel)) {
        SetUIBusy(sel);
        if (!(gTacticalStatus.uiFlags & INCOMBAT) && !(gTacticalStatus.uiFlags & TURNBASED)) {
          BeginDisplayTimedCursor(OKHANDCURSOR_UICURSOR, 300);
        }
        SoldierPickupItem(sel, -1, sIntTileGridNo, bZLevel);
        return -3;
      }
    }
  }

  int16_t sActionGridNo = FindAdjacentGridEx(sel, sIntTileGridNo, NULL, NULL, FALSE, TRUE);
  if (sActionGridNo == -1) sActionGridNo = sIntTileGridNo;

  // If this is not the same tile as ours, check if we can get to dest!
  if (sActionGridNo != sel->sGridNo && gsCurrentActionPoints == 0) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NO_PATH]);
    return -1;
  } else {
    return -2;
  }
}

BOOLEAN HandleUIReloading(SOLDIERTYPE *pSoldier) {
  int8_t bAPs = 0;

  // CHECK OUR CURRENT CURSOR...

  // Do we have the ammo to reload?
  if (guiCurrentUICursor == GOOD_RELOAD_UICURSOR) {
    // Check APs to reload...
    bAPs = GetAPsToAutoReload(pSoldier);

    if (EnoughPoints(pSoldier, bAPs, 0, TRUE)) {
      // OK, we have some ammo we can reload.... reload now!
      if (!AutoReload(pSoldier)) {
        // Do we say we could not reload gun...?
      }

      // ATE: Re-examine cursor info!
      gfUIForceReExamineCursorData = TRUE;
    }
    return (TRUE);
  }

  if (guiCurrentUICursor == BAD_RELOAD_UICURSOR) {
    // OK, we have been told to reload but have no ammo...
    // ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, L"No ammo to reload."
    // );
    if (Random(3) == 0) {
      TacticalCharacterDialogue(pSoldier, QUOTE_OUT_OF_AMMO);
    }
    return (TRUE);
  }

  return (FALSE);
}

BOOLEAN ConfirmActionCancel(uint16_t const usMapPos, uint16_t const usOldMapPos) {
  return usMapPos != usOldMapPos;
}

static void ChangeCurrentSquad(int32_t iSquad) {
  // only allow if nothing in hand and the Change Squad button for whichever
  // panel we're in must be enabled
  if ((gpItemPointer == NULL) && !gfDisableTacticalPanelButtons &&
      (gsCurInterfacePanel != TEAM_PANEL || iTEAMPanelButtons[CHANGE_SQUAD_BUTTON]->Enabled())) {
    if (IsSquadOnCurrentTacticalMap(iSquad)) {
      SetCurrentSquad(iSquad, FALSE);
    }
  }
}

static void HandleSelectMercSlot(uint8_t const ubPanelSlot, bool const force_select) {
  SOLDIERTYPE *const s = GetPlayerFromInterfaceTeamSlot(ubPanelSlot);
  if (s != NULL) {
    HandleLocateSelectMerc(s, force_select);
    ErasePath();
    gfPlotNewMovement = TRUE;
  }
}

static void TestMeanWhile(int32_t iID) {
  int16_t x;
  int16_t y;
  if (iID == INTERROGATION) {
    x = 7;
    y = 14;

    // Loop through our mercs and set gridnos once some found.....
    FOR_EACH_IN_TEAM(s, OUR_TEAM) {
      if (s->bInSector) {
        ChangeSoldiersAssignment(s, ASSIGNMENT_POW);
        s->sSectorX = 7;
        s->sSectorY = 14;
      }
    }
  } else {
    x = 3;
    y = 16;
  }

  ScheduleMeanwhileEvent(x, y, 0, iID, QUEEN, 10);
}

static void EscapeUILock() {
  // UNLOCK UI
  UnSetUIBusy(GetSelectedMan());

  // Decrease global busy  counter...
  gTacticalStatus.ubAttackBusyCount = 0;

  guiPendingOverrideEvent = LU_ENDUILOCK;
  UIHandleLUIEndLock(NULL);
}

#include "TileEngine/MapEdgepoints.h"

static void ToggleMapEdgepoints() {
  static BOOLEAN fToggleEdgepointDisplay = FALSE;
  if (fToggleEdgepointDisplay ^= TRUE) {  // Show edgepoints
    ShowMapEdgepoints();
  } else {  // Hide edgepoints
    HideMapEdgepoints();
  }
  SetRenderFlags(RENDER_FLAG_FULL);
}

void HandleStanceChangeFromUIKeys(uint8_t ubAnimHeight) {
  // If we have multiple guys selected, make all change stance!
  if (gTacticalStatus.fAtLeastOneGuyOnMultiSelect) {
    FOR_EACH_IN_TEAM(s, OUR_TEAM) {
      if (s->bInSector && s->uiStatusFlags & SOLDIER_MULTI_SELECTED) {
        UIHandleSoldierStanceChange(s, ubAnimHeight);
      }
    }
  } else {
    SOLDIERTYPE *const sel = GetSelectedMan();
    if (sel != NULL) UIHandleSoldierStanceChange(sel, ubAnimHeight);
  }
}

static void ToggleStealthMode(SOLDIERTYPE &s) {
  if (gsCurInterfacePanel == SM_PANEL && !giSMStealthButton->Enabled()) return;
  // Either not in SM panel or the matching button is enabled

  if (&s == gpSMCurrentMerc) gfUIStanceDifferent = TRUE;

  gfPlotNewMovement = TRUE;
  fInterfacePanelDirty = DIRTYLEVEL2;
  s.bStealthMode = !s.bStealthMode;

  wchar_t const *const msg = s.bStealthMode ? pMessageStrings[MSG_MERC_ON_STEALTHMODE]
                                            : pMessageStrings[MSG_MERC_OFF_STEALTHMODE];
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, msg, s.name);
}

static void HandleStealthChangeFromUIKeys() {
  // If we have multiple guys selected, make all change stance!
  if (gTacticalStatus.fAtLeastOneGuyOnMultiSelect) {
    FOR_EACH_IN_TEAM(s, OUR_TEAM) {
      if (AM_A_ROBOT(s)) continue;
      if (!s->bInSector) continue;
      if (!(s->uiStatusFlags & SOLDIER_MULTI_SELECTED)) continue;
      ToggleStealthMode(*s);
    }
  } else {
    SOLDIERTYPE *const sel = GetSelectedMan();
    if (sel && !AM_A_ROBOT(sel)) ToggleStealthMode(*sel);
  }
}

static void TestCapture() {
  BeginCaptureSquence();

  gStrategicStatus.uiFlags &= ~STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE;

  // Loop through sodliers and pick 3 lucky ones
  uint32_t n = 3;
  FOR_EACH_IN_TEAM(i, OUR_TEAM) {
    SOLDIERTYPE &s = *i;
    if (s.bLife < OKLIFE) continue;
    if (!s.bInSector) continue;
    EnemyCapturesPlayerSoldier(&s);
    RemoveSoldierFromGridNo(s);
    if (--n == 0) break;
  }

  EndCaptureSequence();
}

void PopupAssignmentMenuInTactical() {
  // do something
  fShowAssignmentMenu = TRUE;
  CreateDestroyAssignmentPopUpBoxes();
  SetTacticalPopUpAssignmentBoxXY();
  DetermineBoxPositions();
  DetermineWhichAssignmentMenusCanBeShown();
  fFirstClickInAssignmentScreenMask = TRUE;
  gfIgnoreScrolling = TRUE;
}
