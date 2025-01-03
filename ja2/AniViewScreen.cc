// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "AniViewScreen.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/MemMan.h"
#include "SGP/Video.h"
#include "SysGlobals.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierAni.h"
#include "Tactical/SoldierControl.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"

#include "SDL_keycode.h"

BOOLEAN gfAniEditMode = FALSE;
static uint16_t usStartAnim = 0;
static uint8_t ubStartHeight = 0;
static SOLDIERTYPE *pSoldier;

static BOOLEAN fOKFiles = FALSE;
static uint8_t ubNumStates = 0;
static uint16_t *pusStates = NULL;
static int8_t ubCurLoadedState = 0;

static void CycleAnimations() {
  int32_t cnt;

  // FInd the next animation with start height the same...
  for (cnt = usStartAnim + 1; cnt < NUMANIMATIONSTATES; cnt++) {
    if (gAnimControl[cnt].ubHeight == ubStartHeight) {
      usStartAnim = (uint8_t)cnt;
      EVENT_InitNewSoldierAnim(pSoldier, usStartAnim, 0, TRUE);
      return;
    }
  }

  usStartAnim = 0;
  EVENT_InitNewSoldierAnim(pSoldier, usStartAnim, 0, TRUE);
}

static void BuildListFile();

ScreenID AniEditScreenHandle() {
  InputAtom InputEvent;
  static BOOLEAN fFirstTime = TRUE;
  static uint16_t usOldState;
  static BOOLEAN fToggle = FALSE;
  static BOOLEAN fToggle2 = FALSE;

  // Make backups
  if (fFirstTime) {
    gfAniEditMode = TRUE;

    usStartAnim = 0;
    ubStartHeight = ANIM_STAND;

    fFirstTime = FALSE;
    fToggle = FALSE;
    fToggle2 = FALSE;
    ubCurLoadedState = 0;

    pSoldier = GetSelectedMan();

    gTacticalStatus.uiFlags |= LOADING_SAVED_GAME;

    EVENT_InitNewSoldierAnim(pSoldier, usStartAnim, 0, TRUE);

    BuildListFile();
  }

  RenderWorld();
  ExecuteBaseDirtyRectQueue();
  EndFrameBufferRender();

  SetFont(LARGEFONT1);
  GPrintDirty(0, 0, L"SOLDIER ANIMATION VIEWER");
  GPrintDirtyF(0, 20, L"Current Animation: %hs %hs", gAnimControl[usStartAnim].zAnimStr,
               gAnimSurfaceDatabase[pSoldier->usAnimSurface].Filename);

  switch (ubStartHeight) {
    case ANIM_STAND:
      GPrintDirty(0, 40, L"Current Stance: STAND");
      break;
    case ANIM_CROUCH:
      GPrintDirty(0, 40, L"Current Stance: CROUCH");
      break;
    case ANIM_PRONE:
      GPrintDirty(0, 40, L"Current Stance: PRONE");
      break;
  }

  if (fToggle) {
    GPrintDirty(0, 60, L"FORCE ON");
  }

  if (fToggle2) {
    GPrintDirty(0, 70, L"LOADED ORDER ON");
    GPrintDirtyF(0, 90, L"LOADED ORDER : %hs", gAnimControl[pusStates[ubCurLoadedState]].zAnimStr);
  }

  if (DequeueEvent(&InputEvent)) {
    if (InputEvent.usEvent == KEY_DOWN && InputEvent.usParam == SDLK_ESCAPE) {
      fFirstTime = TRUE;

      gfAniEditMode = FALSE;

      fFirstTimeInGameScreen = TRUE;

      gTacticalStatus.uiFlags &= (~LOADING_SAVED_GAME);

      if (fOKFiles) {
        MemFree(pusStates);
      }

      fOKFiles = FALSE;

      return (GAME_SCREEN);
    }

    if (InputEvent.usEvent == KEY_UP && InputEvent.usParam == SDLK_SPACE) {
      if (!fToggle && !fToggle2) {
        CycleAnimations();
      }
    }

    if ((InputEvent.usEvent == KEY_UP) && (InputEvent.usParam == 's')) {
      if (!fToggle) {
        uint16_t usAnim = 0;
        usOldState = usStartAnim;

        switch (ubStartHeight) {
          case ANIM_STAND:

            usAnim = STANDING;
            break;

          case ANIM_CROUCH:

            usAnim = CROUCHING;
            break;

          case ANIM_PRONE:

            usAnim = PRONE;
            break;
        }

        EVENT_InitNewSoldierAnim(pSoldier, usAnim, 0, TRUE);
      } else {
        EVENT_InitNewSoldierAnim(pSoldier, usOldState, 0, TRUE);
      }

      fToggle = !fToggle;
    }

    if ((InputEvent.usEvent == KEY_UP) && (InputEvent.usParam == 'l')) {
      if (!fToggle2) {
        usOldState = usStartAnim;

        EVENT_InitNewSoldierAnim(pSoldier, pusStates[ubCurLoadedState], 0, TRUE);
      } else {
        EVENT_InitNewSoldierAnim(pSoldier, usOldState, 0, TRUE);
      }

      fToggle2 = !fToggle2;
    }

    if (InputEvent.usEvent == KEY_UP && InputEvent.usParam == SDLK_PAGEUP) {
      if (fOKFiles && fToggle2) {
        ubCurLoadedState++;

        if (ubCurLoadedState == ubNumStates) {
          ubCurLoadedState = 0;
        }

        EVENT_InitNewSoldierAnim(pSoldier, pusStates[ubCurLoadedState], 0, TRUE);
      }
    }

    if (InputEvent.usEvent == KEY_UP && InputEvent.usParam == SDLK_PAGEDOWN) {
      if (fOKFiles && fToggle2) {
        ubCurLoadedState--;

        if (ubCurLoadedState == 0) {
          ubCurLoadedState = ubNumStates;
        }

        EVENT_InitNewSoldierAnim(pSoldier, pusStates[ubCurLoadedState], 0, TRUE);
      }
    }

    if ((InputEvent.usEvent == KEY_UP) && (InputEvent.usParam == 'c')) {
      // CLEAR!
      usStartAnim = 0;
      EVENT_InitNewSoldierAnim(pSoldier, usStartAnim, 0, TRUE);
    }

    if (InputEvent.usEvent == KEY_UP && InputEvent.usParam == SDLK_RETURN) {
      if (ubStartHeight == ANIM_STAND) {
        ubStartHeight = ANIM_CROUCH;
      } else if (ubStartHeight == ANIM_CROUCH) {
        ubStartHeight = ANIM_PRONE;
      } else {
        ubStartHeight = ANIM_STAND;
      }
    }
  }

  return (ANIEDIT_SCREEN);
}

static uint16_t GetAnimStateFromName(const char *zName) {
  int32_t cnt;

  // FInd the next animation with start height the same...
  for (cnt = 0; cnt < NUMANIMATIONSTATES; cnt++) {
    if (strcasecmp(gAnimControl[cnt].zAnimStr, zName) == 0) return cnt;
  }

  return (5555);
}

static void BuildListFile() {
  FILE *infoFile;
  char currFilename[128];
  int numEntries = 0;
  int cnt;
  uint16_t usState;
  wchar_t zError[128];

  // Verify the existance of the header text file.
  infoFile = fopen("anitest.dat", "rb");
  if (!infoFile) {
    return;
  }
  // count STIs inside header and verify each one's existance.
  while (!feof(infoFile)) {
    fgets(currFilename, 128, infoFile);
    // valid entry in header, continue on...

    numEntries++;
  }
  fseek(infoFile, 0, SEEK_SET);  // reset header file

  // Allocate array
  pusStates = MALLOCN(uint16_t, numEntries);

  fOKFiles = TRUE;

  cnt = 0;
  while (!feof(infoFile)) {
    fgets(currFilename, 128, infoFile);

    // Remove newline
    currFilename[strlen(currFilename) - 1] = '\0';
    currFilename[strlen(currFilename) - 1] = '\0';

    usState = GetAnimStateFromName(currFilename);

    if (usState != 5555) {
      cnt++;
      ubNumStates = (uint8_t)cnt;
      pusStates[cnt] = usState;
    } else {
      swprintf(zError, lengthof(zError), L"Animation str %hs is not known: ", currFilename);
      DoMessageBox(MSG_BOX_BASIC_STYLE, zError, ANIEDIT_SCREEN, MSG_BOX_FLAG_YESNO, NULL, NULL);
      fclose(infoFile);
      return;
    }
  }
  fclose(infoFile);
}
