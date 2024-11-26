#include "TileEngine/TacticalPlacementGUI.h"

#include <stdexcept>

#include "Directories.h"
#include "GameLoop.h"
#include "JAScreens.h"
#include "Local.h"
#include "MercPortrait.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/CursorControl.h"
#include "SGP/Debug.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/Line.h"
#include "SGP/MemMan.h"
#include "SGP/MouseSystem.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Strategic/Assignments.h"
#include "Strategic/GameClock.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/Interface.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierAdd.h"
#include "Tactical/SoldierProfile.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/MapEdgepoints.h"
#include "TileEngine/OverheadMap.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#include "SDL_keycode.h"

struct MERCPLACEMENT {
  SOLDIERTYPE *pSoldier;
  SGPVObject *uiVObjectID;
  MOUSE_REGION region;
  uint8_t ubStrategicInsertionCode;
  BOOLEAN fPlaced;
};

static MERCPLACEMENT *gMercPlacement = 0;
static int32_t giPlacements = 0;

#define FOR_EACH_MERC_PLACEMENT(iter)                                                            \
  for (MERCPLACEMENT *iter = gMercPlacement, *const iter##__end = gMercPlacement + giPlacements; \
       iter != iter##__end; ++i)

enum { DONE_BUTTON, SPREAD_BUTTON, GROUP_BUTTON, CLEAR_BUTTON, NUM_TP_BUTTONS };
GUIButtonRef iTPButtons[NUM_TP_BUTTONS];

uint8_t gubDefaultButton = CLEAR_BUTTON;
BOOLEAN gfTacticalPlacementGUIActive = FALSE;
BOOLEAN gfTacticalPlacementFirstTime = FALSE;
BOOLEAN gfEnterTacticalPlacementGUI = FALSE;
BOOLEAN gfKillTacticalGUI = FALSE;
static SGPVObject *giOverheadPanelImage;
static BUTTON_PICS *giOverheadButtonImages[NUM_TP_BUTTONS];
SGPVObject *giMercPanelImage = 0;
BOOLEAN gfTacticalPlacementGUIDirty = FALSE;
BOOLEAN gfValidLocationsChanged = FALSE;
BOOLEAN gfValidCursor = FALSE;
BOOLEAN gfEveryonePlaced = FALSE;

uint8_t gubSelectedGroupID = 0;
uint8_t gubHilightedGroupID = 0;
uint8_t gubCursorGroupID = 0;
int8_t gbSelectedMercID = -1;
int8_t gbHilightedMercID = -1;
int8_t gbCursorMercID = -1;
SOLDIERTYPE *gpTacticalPlacementSelectedSoldier = NULL;
SOLDIERTYPE *gpTacticalPlacementHilightedSoldier = NULL;

static bool gfNorth;
static bool gfEast;
static bool gfSouth;
static bool gfWest;

static void MakeButton(uint32_t idx, int16_t y, GUI_CALLBACK click, const wchar_t *text,
                       const wchar_t *help) {
  GUIButtonRef const btn =
      QuickCreateButton(giOverheadButtonImages[idx], 11, y, MSYS_PRIORITY_HIGH, click);
  iTPButtons[idx] = btn;
  btn->SpecifyGeneralTextAttributes(text, BLOCKFONT, FONT_BEIGE, 141);
  btn->SetFastHelpText(help);
  btn->SpecifyHilitedTextColors(FONT_WHITE, FONT_NEARBLACK);
}

static void ClearPlacementsCallback(GUI_BUTTON *btn, int32_t reason);
static void DoneOverheadPlacementClickCallback(GUI_BUTTON *btn, int32_t reason);
static void GroupPlacementsCallback(GUI_BUTTON *btn, int32_t reason);
static void MercClickCallback(MOUSE_REGION *reg, int32_t reason);
static void MercMoveCallback(MOUSE_REGION *reg, int32_t reason);
static void PlaceMercs();
static void SetCursorMerc(int8_t placement);
static void SpreadPlacementsCallback(GUI_BUTTON *btn, int32_t reason);

void InitTacticalPlacementGUI() {
  gfTacticalPlacementGUIActive = TRUE;
  gfTacticalPlacementGUIDirty = TRUE;
  gfValidLocationsChanged = TRUE;
  gfTacticalPlacementFirstTime = TRUE;

  GoIntoOverheadMap();

  giOverheadPanelImage = AddVideoObjectFromFile(INTERFACEDIR "/overheadinterface.sti");
  giMercPanelImage = AddVideoObjectFromFile(INTERFACEDIR "/panels.sti");

  BUTTON_PICS *const img = LoadButtonImage(INTERFACEDIR "/overheaduibuttons.sti", 0, 1);
  giOverheadButtonImages[DONE_BUTTON] = img;
  giOverheadButtonImages[SPREAD_BUTTON] = UseLoadedButtonImage(img, 0, 1);
  giOverheadButtonImages[GROUP_BUTTON] = UseLoadedButtonImage(img, 0, 1);
  giOverheadButtonImages[CLEAR_BUTTON] = UseLoadedButtonImage(img, 0, 1);

  // Create the buttons which provide automatic placements.
  MakeButton(CLEAR_BUTTON, 332, ClearPlacementsCallback, gpStrategicString[STR_TP_CLEAR],
             gpStrategicString[STR_TP_CLEARHELP]);
  MakeButton(SPREAD_BUTTON, 367, SpreadPlacementsCallback, gpStrategicString[STR_TP_SPREAD],
             gpStrategicString[STR_TP_SPREADHELP]);
  MakeButton(GROUP_BUTTON, 402, GroupPlacementsCallback, gpStrategicString[STR_TP_GROUP],
             gpStrategicString[STR_TP_GROUPHELP]);
  MakeButton(DONE_BUTTON, 437, DoneOverheadPlacementClickCallback, gpStrategicString[STR_TP_DONE],
             gpStrategicString[STR_TP_DONEHELP]);
  iTPButtons[DONE_BUTTON]->AllowDisabledFastHelp();

  GROUP const &bg = *gpBattleGroup;
  /* First pass: Count the number of mercs that are going to be placed by the
   * player. This determines the size of the array we will allocate. */
  size_t n = 0;
  CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s->fBetweenSectors) continue;
    if (s->sSectorX != bg.ubSectorX) continue;
    if (s->sSectorY != bg.ubSectorY) continue;
    if (s->uiStatusFlags & SOLDIER_VEHICLE) continue;  // ATE Ignore vehicles
    if (s->bAssignment == ASSIGNMENT_POW) continue;
    if (s->bAssignment == IN_TRANSIT) continue;
    if (s->bSectorZ != 0) continue;
    ++n;
  }
  // Allocate the array based on how many mercs there are.
  gMercPlacement = MALLOCNZ(MERCPLACEMENT, n);

  // Second pass: Assign the mercs to their respective slots.
  giPlacements = 0;
  gfNorth = false;
  gfEast = false;
  gfSouth = false;
  gfWest = false;
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s->bLife == 0) continue;
    if (s->fBetweenSectors) continue;
    if (s->sSectorX != bg.ubSectorX) continue;
    if (s->sSectorY != bg.ubSectorY) continue;
    if (s->uiStatusFlags & SOLDIER_VEHICLE) continue;  // ATE Ignore vehicles
    if (s->bAssignment == ASSIGNMENT_POW) continue;
    if (s->bAssignment == IN_TRANSIT) continue;
    if (s->bSectorZ != 0) continue;

    if (s->ubStrategicInsertionCode == INSERTION_CODE_PRIMARY_EDGEINDEX ||
        s->ubStrategicInsertionCode == INSERTION_CODE_SECONDARY_EDGEINDEX) {
      s->ubStrategicInsertionCode = (uint8_t)s->usStrategicInsertionData;
    }

    uint32_t const i = giPlacements++;
    MERCPLACEMENT &m = gMercPlacement[i];
    m.pSoldier = s;
    m.ubStrategicInsertionCode = s->ubStrategicInsertionCode;
    m.fPlaced = FALSE;
    m.uiVObjectID = Load65Portrait(GetProfile(m.pSoldier->ubProfile));
    int32_t const x = 91 + i / 2 * 54;
    int32_t const y = 361 + i % 2 * 51;
    MSYS_DefineRegion(&m.region, x, y, x + 54, y + 62, MSYS_PRIORITY_HIGH, 0, MercMoveCallback,
                      MercClickCallback);

    switch (s->ubStrategicInsertionCode) {
      case INSERTION_CODE_NORTH:
        gfNorth = true;
        break;
      case INSERTION_CODE_EAST:
        gfEast = true;
        break;
      case INSERTION_CODE_SOUTH:
        gfSouth = true;
        break;
      case INSERTION_CODE_WEST:
        gfWest = true;
        break;
    }
  }

  PlaceMercs();

  if (gubDefaultButton == GROUP_BUTTON) {
    iTPButtons[GROUP_BUTTON]->uiFlags |= BUTTON_CLICKED_ON;
    for (int32_t i = 0; i != giPlacements; ++i) {
      MERCPLACEMENT const &m = gMercPlacement[i];
      if (m.fPlaced) continue;
      // Found an unplaced merc. Select him.
      gbSelectedMercID = i;
      gubSelectedGroupID = m.pSoldier->ubGroupID;
      gpTacticalPlacementSelectedSoldier = m.pSoldier;
      SetCursorMerc(i);
      break;
    }
  }
}

static void DrawBar(SGPVSurface *const buf, int32_t const x, int32_t const y, int32_t const h,
                    uint32_t const colour1, uint32_t const colour2) {
  ColorFillVideoSurfaceArea(buf, x, y - h, x + 1, y, Get16BPPColor(colour1));
  ColorFillVideoSurfaceArea(buf, x + 1, y - h, x + 2, y, Get16BPPColor(colour2));
}

static void RenderTacticalPlacementGUI() {
  if (gfTacticalPlacementFirstTime) {
    gfTacticalPlacementFirstTime = FALSE;
    DisableScrollMessages();
  }

  /* Check to make sure that if we have a hilighted merc (not selected) and the
   * mouse has moved out of its region, then we will clear the hilighted ID, and
   * refresh the display. */
  if (!gfTacticalPlacementGUIDirty && gbHilightedMercID != -1) {
    int32_t const x = 91 + gbHilightedMercID / 2 * 54;
    int32_t const y = 361 + gbHilightedMercID % 2 * 51;
    if (gusMouseXPos < x || x + 54 < gusMouseXPos || gusMouseYPos < y || y + 62 < gusMouseYPos) {
      gbHilightedMercID = -1;
      gubHilightedGroupID = 0;
      SetCursorMerc(gbSelectedMercID);
      gpTacticalPlacementHilightedSoldier = 0;
    }
  }

  SGPVSurface *const buf = FRAME_BUFFER;
  // If the display is dirty render the entire panel.
  if (gfTacticalPlacementGUIDirty) {
    BltVideoObject(buf, giOverheadPanelImage, 0, 0, 320);
    InvalidateRegion(0, 0, 320, SCREEN_HEIGHT);
    gfTacticalPlacementGUIDirty = FALSE;
    MarkButtonsDirty();
    for (int32_t i = 0; i != giPlacements; ++i) {  // Render the mercs
      MERCPLACEMENT const &m = gMercPlacement[i];
      int32_t const x = 95 + i / 2 * 54;
      int32_t const y = 371 + i % 2 * 51;
      ColorFillVideoSurfaceArea(buf, x + 36, y + 2, x + 44, y + 30, 0);
      BltVideoObject(buf, giMercPanelImage, 0, x, y);
      BltVideoObject(buf, m.uiVObjectID, 0, x + 2, y + 2);

      SOLDIERTYPE const &s = *m.pSoldier;
      if (s.bLife == 0) continue;

      DrawBar(buf, x + 36, y + 29, s.bLifeMax * 27 / 100, FROMRGB(107, 107, 57),
              FROMRGB(222, 181, 115));  // Yellow one for bleeding
      DrawBar(buf, x + 36, y + 29, s.bBleeding * 27 / 100, FROMRGB(156, 57, 57),
              FROMRGB(222, 132, 132));  // Pink one for bandaged
      DrawBar(buf, x + 36, y + 29, s.bLife * 27 / 100, FROMRGB(107, 8, 8),
              FROMRGB(206, 0, 0));  // Red one for actual health
      DrawBar(buf, x + 39, y + 29, s.bBreathMax * 27 / 100, FROMRGB(8, 8, 132),
              FROMRGB(8, 8, 107));  // Breath bar
      DrawBar(buf, x + 42, y + 29, s.bMorale * 27 / 100, FROMRGB(8, 156, 8),
              FROMRGB(8, 107, 8));  // Morale bar
    }

    SetFontAttributes(BLOCKFONT, FONT_BEIGE);
    wchar_t str[128];
    GetSectorIDString(gubPBSectorX, gubPBSectorY, gubPBSectorZ, str, lengthof(str), TRUE);
    mprintf(120, 335, L"%ls %ls -- %ls...", gpStrategicString[STR_TP_SECTOR], str,
            gpStrategicString[STR_TP_CHOOSEENTRYPOSITIONS]);

    // Shade out the part of the tactical map that isn't considered placable.
    BlitBufferToBuffer(buf, guiSAVEBUFFER, 0, 320, SCREEN_WIDTH, 160);
  }

  if (gfValidLocationsChanged) {
    gfValidLocationsChanged = FALSE;
    BlitBufferToBuffer(guiSAVEBUFFER, buf, 4, 4, 636, 320);
    InvalidateRegion(4, 4, 636, 320);

    uint16_t const hatch_colour =
        DayTime() ? 0 :  // 6AM to 9PM is black
            Get16BPPColor(FROMRGB(63, 31,
                                  31));  // 9PM to 6AM is gray (black is too dark to distinguish)
    SGPRect clip = {4, 4, 636, 320};
    if (gbCursorMercID == -1) {
      if (gfNorth) clip.iTop = 30;
      if (gfEast) clip.iRight = 610;
      if (gfSouth) clip.iBottom = 290;
      if (gfWest) clip.iLeft = 30;
    } else {
      switch (gMercPlacement[gbCursorMercID].ubStrategicInsertionCode) {
        case INSERTION_CODE_NORTH:
          clip.iTop = 30;
          break;
        case INSERTION_CODE_EAST:
          clip.iRight = 610;
          break;
        case INSERTION_CODE_SOUTH:
          clip.iBottom = 290;
          break;
        case INSERTION_CODE_WEST:
          clip.iLeft = 30;
          break;
      }
    }
    SGPVSurface::Lock l(buf);
    uint16_t *const pDestBuf = l.Buffer<uint16_t>();
    uint32_t const uiDestPitchBYTES = l.Pitch();
    Blt16BPPBufferLooseHatchRectWithColor(pDestBuf, uiDestPitchBYTES, &clip, hatch_colour);
    SetClippingRegionAndImageWidth(uiDestPitchBYTES, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    RectangleDraw(TRUE, clip.iLeft, clip.iTop, clip.iRight, clip.iBottom, hatch_colour, pDestBuf);
  }

  bool const is_group = gubDefaultButton == GROUP_BUTTON;
  for (int32_t i = 0; i != giPlacements; ++i) {  // Render the merc's names
    int32_t const x = 95 + i / 2 * 54;
    int32_t const y = 371 + i % 2 * 51;

    MERCPLACEMENT const &m = gMercPlacement[i];
    SOLDIERTYPE const &s = *m.pSoldier;
    uint8_t const colour =
        (is_group ? s.ubGroupID == gubSelectedGroupID : i == gbSelectedMercID)     ? FONT_YELLOW
        : (is_group ? s.ubGroupID == gubHilightedGroupID : i == gbHilightedMercID) ? FONT_WHITE
                                                                                   : FONT_GRAY3;
    SetFontAttributes(BLOCKFONT, colour);
    int32_t const w = StringPixLength(s.name, BLOCKFONT);
    int32_t const nx = x + (48 - w) / 2;
    int32_t const ny = y + 33;
    MPrint(nx, ny, s.name);
    InvalidateRegion(nx, ny, nx + w, ny + w);

    // Render a question mark over the face, if the merc hasn't yet been placed.
    int32_t const qx = x + 16;
    int32_t const qy = y + 14;
    if (m.fPlaced) {
      RegisterBackgroundRect(BGND_FLAG_SINGLE, qx, qy, 8, 8);
    } else {
      SetFont(FONT10ARIALBOLD);
      MPrint(qx, qy, L"?");
      InvalidateRegion(qx, qy, qx + 8, qy + 8);
    }
  }
}

static void EnsureDoneButtonStatus() {
  bool enable = true;
  // static BOOLEAN fInside = FALSE;
  // BOOLEAN fChanged = FALSE;
  FOR_EACH_MERC_PLACEMENT(i) {
    if (i->fPlaced) continue;
    enable = false;
    break;
  }
  GUI_BUTTON &b = *iTPButtons[DONE_BUTTON];
  // Only enable it when it is disabled, otherwise the button will stay down
  if (b.Enabled() == enable) return;
  EnableButton(&b, enable);
  b.SetFastHelpText(enable ? gpStrategicString[STR_TP_DONEHELP]
                           : gpStrategicString[STR_TP_DISABLED_DONEHELP]);
}

static void KillTacticalPlacementGUI();

void TacticalPlacementHandle() {
  InputAtom InputEvent;

  EnsureDoneButtonStatus();

  RenderTacticalPlacementGUI();

  if (gfRightButtonState) {
    gbSelectedMercID = -1;
    gubSelectedGroupID = 0;
    gpTacticalPlacementSelectedSoldier = NULL;
  }

  while (DequeueEvent(&InputEvent)) {
    if (InputEvent.usEvent == KEY_DOWN) {
      switch (InputEvent.usParam) {
        case SDLK_RETURN:
          if (iTPButtons[DONE_BUTTON]->Enabled()) {
            KillTacticalPlacementGUI();
          }
          break;
        case 'c':
          ClearPlacementsCallback(iTPButtons[CLEAR_BUTTON], MSYS_CALLBACK_REASON_LBUTTON_UP);
          break;
        case 'g':
          GroupPlacementsCallback(iTPButtons[GROUP_BUTTON], MSYS_CALLBACK_REASON_LBUTTON_UP);
          break;
        case 's':
          SpreadPlacementsCallback(iTPButtons[SPREAD_BUTTON], MSYS_CALLBACK_REASON_LBUTTON_UP);
          break;
        case 'x':
          if (InputEvent.usKeyState & ALT_DOWN) {
            HandleShortCutExitState();
          }
          break;
      }
    }
  }
  gfValidCursor = FALSE;
  if (gbSelectedMercID != -1 && gusMouseYPos < 320) {
    switch (gMercPlacement[gbCursorMercID].ubStrategicInsertionCode) {
      case INSERTION_CODE_NORTH:
        if (gusMouseYPos <= 40) gfValidCursor = TRUE;
        break;
      case INSERTION_CODE_EAST:
        if (gusMouseXPos >= 600) gfValidCursor = TRUE;
        break;
      case INSERTION_CODE_SOUTH:
        if (gusMouseYPos >= 280) gfValidCursor = TRUE;
        break;
      case INSERTION_CODE_WEST:
        if (gusMouseXPos <= 40) gfValidCursor = TRUE;
        break;
    }
    if (gubDefaultButton == GROUP_BUTTON) {
      if (gfValidCursor) {
        SetCurrentCursorFromDatabase(CURSOR_PLACEGROUP);
      } else {
        SetCurrentCursorFromDatabase(CURSOR_DPLACEGROUP);
      }
    } else {
      if (gfValidCursor) {
        SetCurrentCursorFromDatabase(CURSOR_PLACEMERC);
      } else {
        SetCurrentCursorFromDatabase(CURSOR_DPLACEMERC);
      }
    }
  } else {
    SetCurrentCursorFromDatabase(CURSOR_NORMAL);
  }
  if (gfKillTacticalGUI == 1) {
    KillTacticalPlacementGUI();
  } else if (gfKillTacticalGUI == 2) {
    gfKillTacticalGUI = 1;
  }
}

static void PickUpMercPiece(MERCPLACEMENT &);

static void KillTacticalPlacementGUI() {
  gbHilightedMercID = -1;
  gbSelectedMercID = -1;
  gubSelectedGroupID = 0;
  gubHilightedGroupID = 0;
  gbCursorMercID = -1;
  gpTacticalPlacementHilightedSoldier = NULL;
  gpTacticalPlacementSelectedSoldier = NULL;

  // Destroy the tactical placement gui.
  gfEnterTacticalPlacementGUI = FALSE;
  gfTacticalPlacementGUIActive = FALSE;
  gfKillTacticalGUI = FALSE;
  // Delete video objects
  DeleteVideoObject(giOverheadPanelImage);
  DeleteVideoObject(giMercPanelImage);
  // Delete buttons
  for (int32_t i = 0; i < NUM_TP_BUTTONS; ++i) {
    UnloadButtonImage(giOverheadButtonImages[i]);
    RemoveButton(iTPButtons[i]);
  }
  // Delete faces and regions
  FOR_EACH_MERC_PLACEMENT(i) {
    MERCPLACEMENT &m = *i;
    DeleteVideoObject(m.uiVObjectID);
    MSYS_RemoveRegion(&m.region);
  }

  if (gsCurInterfacePanel < 0 || gsCurInterfacePanel >= NUM_UI_PANELS)
    gsCurInterfacePanel = TEAM_PANEL;

  SetCurrentInterfacePanel(gsCurInterfacePanel);

  // Leave the overhead map.
  KillOverheadMap();
  // Recreate the tactical panel.
  gRadarRegion.Enable();
  SetCurrentInterfacePanel(TEAM_PANEL);
  // Initialize the rest of the map (AI, enemies, civs, etc.)

  FOR_EACH_MERC_PLACEMENT(i) PickUpMercPiece(*i);

  PrepareLoadedSector();
  EnableScrollMessages();
}

static void PutDownMercPiece(MERCPLACEMENT &);

static void ChooseRandomEdgepoints() {
  FOR_EACH_MERC_PLACEMENT(i) {
    MERCPLACEMENT &m = *i;
    if (!(m.pSoldier->uiStatusFlags & SOLDIER_VEHICLE)) {
      m.pSoldier->usStrategicInsertionData = ChooseMapEdgepoint(m.ubStrategicInsertionCode);
      if (m.pSoldier->usStrategicInsertionData != NOWHERE) {
        m.pSoldier->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
      } else {
#if 0 /* XXX unsigned < 0 ? */
				Assert(0 <= m.pSoldier->usStrategicInsertionData && m.pSoldier->usStrategicInsertionData < WORLD_MAX);
#else
        Assert(m.pSoldier->usStrategicInsertionData < WORLD_MAX);
#endif
        m.pSoldier->ubStrategicInsertionCode = m.ubStrategicInsertionCode;
      }
    }

    PutDownMercPiece(m);
  }
  gfEveryonePlaced = TRUE;
}

static void PlaceMercs() {
  switch (gubDefaultButton) {
    case SPREAD_BUTTON:  // Place mercs randomly along their side using map
                         // edgepoints.
      ChooseRandomEdgepoints();
      break;
    case CLEAR_BUTTON:
      FOR_EACH_MERC_PLACEMENT(i) PickUpMercPiece(*i);
      gubSelectedGroupID = 0;
      gbSelectedMercID = 0;
      SetCursorMerc(0);
      gfEveryonePlaced = FALSE;
      break;
    default:
      return;
  }
  gfTacticalPlacementGUIDirty = TRUE;
}

static void DoneOverheadPlacementClickCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gfKillTacticalGUI = 2;
  }
}

static void SpreadPlacementsCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gubDefaultButton = SPREAD_BUTTON;
    iTPButtons[GROUP_BUTTON]->uiFlags &= ~BUTTON_CLICKED_ON;
    iTPButtons[GROUP_BUTTON]->uiFlags |= BUTTON_DIRTY;
    PlaceMercs();
    gubSelectedGroupID = 0;
    gbSelectedMercID = -1;
    SetCursorMerc(-1);
  }
}

static void GroupPlacementsCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (gubDefaultButton == GROUP_BUTTON) {
      btn->uiFlags &= ~BUTTON_CLICKED_ON;
      btn->uiFlags |= BUTTON_DIRTY;
      gubDefaultButton = CLEAR_BUTTON;
      gubSelectedGroupID = 0;
    } else {
      btn->uiFlags |= BUTTON_CLICKED_ON | BUTTON_DIRTY;
      gubDefaultButton = GROUP_BUTTON;
      gbSelectedMercID = 0;
      SetCursorMerc(gbSelectedMercID);
      gubSelectedGroupID = gMercPlacement[gbSelectedMercID].pSoldier->ubGroupID;
    }
  }
}

static void ClearPlacementsCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iTPButtons[GROUP_BUTTON]->uiFlags &= ~BUTTON_CLICKED_ON;
    iTPButtons[GROUP_BUTTON]->uiFlags |= BUTTON_DIRTY;
    gubDefaultButton = CLEAR_BUTTON;
    PlaceMercs();
  }
}

static void MercMoveCallback(MOUSE_REGION *reg, int32_t reason) {
  if (reg->uiFlags & MSYS_MOUSE_IN_AREA) {
    int8_t i;
    for (i = 0; i < giPlacements; i++) {
      if (&gMercPlacement[i].region == reg) {
        if (gbHilightedMercID != i) {
          gbHilightedMercID = i;
          if (gubDefaultButton == GROUP_BUTTON)
            gubHilightedGroupID = gMercPlacement[i].pSoldier->ubGroupID;
          SetCursorMerc(i);
          gpTacticalPlacementHilightedSoldier = gMercPlacement[i].pSoldier;
        }
        return;
      }
    }
  }
}

static void MercClickCallback(MOUSE_REGION *reg, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    int8_t i;
    for (i = 0; i < giPlacements; i++) {
      if (&gMercPlacement[i].region == reg) {
        if (gbSelectedMercID != i) {
          gbSelectedMercID = i;
          gpTacticalPlacementSelectedSoldier = gMercPlacement[i].pSoldier;
          if (gubDefaultButton == GROUP_BUTTON) {
            gubSelectedGroupID = gpTacticalPlacementSelectedSoldier->ubGroupID;
          }
        }
        return;
      }
    }
  }
}

static void SelectNextUnplacedUnit() {
  int32_t i;
  if (gbSelectedMercID == -1) return;
  for (i = gbSelectedMercID; i < giPlacements;
       i++) {                          // go from the currently selected soldier to the end
    if (!gMercPlacement[i].fPlaced) {  // Found an unplaced merc.  Select him.
      gbSelectedMercID = (int8_t)i;
      if (gubDefaultButton == GROUP_BUTTON)
        gubSelectedGroupID = gMercPlacement[i].pSoldier->ubGroupID;
      gfTacticalPlacementGUIDirty = TRUE;
      SetCursorMerc((int8_t)i);
      gpTacticalPlacementSelectedSoldier = gMercPlacement[i].pSoldier;
      return;
    }
  }
  for (i = 0; i < gbSelectedMercID;
       i++) {                          // go from the beginning to the currently selected soldier
    if (!gMercPlacement[i].fPlaced) {  // Found an unplaced merc.  Select him.
      gbSelectedMercID = (int8_t)i;
      if (gubDefaultButton == GROUP_BUTTON)
        gubSelectedGroupID = gMercPlacement[i].pSoldier->ubGroupID;
      gfTacticalPlacementGUIDirty = TRUE;
      SetCursorMerc((int8_t)i);
      gpTacticalPlacementSelectedSoldier = gMercPlacement[i].pSoldier;
      return;
    }
  }
  // checked the whole array, and everybody has been placed.  Select nobody.
  if (!gfEveryonePlaced) {
    gfEveryonePlaced = TRUE;
    SetCursorMerc(-1);
    gbSelectedMercID = -1;
    gubSelectedGroupID = 0;
    gfTacticalPlacementGUIDirty = TRUE;
    gfValidLocationsChanged = TRUE;
    gpTacticalPlacementSelectedSoldier = gMercPlacement[i].pSoldier;
  }
}

static void DialogRemoved(MessageBoxReturnValue);

void HandleTacticalPlacementClicksInOverheadMap(int32_t reason) {
  BOOLEAN fInvalidArea = FALSE;
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {  // if we have a selected merc,
                                                   // move him to the new closest
                                                   // map edgepoint of his side.
    if (gfValidCursor) {
      if (gbSelectedMercID != -1) {
        const GridNo sGridNo = GetOverheadMouseGridNo();
        if (sGridNo != NOWHERE) {  // we have clicked within a valid part of the map.
          BeginMapEdgepointSearch();

          if (gubDefaultButton == GROUP_BUTTON) {  // We are placing a whole group.
            FOR_EACH_MERC_PLACEMENT(i) {           // Find locations of each member of the
                                                   // group, but don't place them yet.  If
              // one of the mercs can't be placed, then we won't place any, and
              // tell the user the problem.  If everything's okay, we will place
              // them all.
              MERCPLACEMENT const &m = *i;
              if (m.pSoldier->ubGroupID == gubSelectedGroupID) {
                m.pSoldier->usStrategicInsertionData =
                    SearchForClosestPrimaryMapEdgepoint(sGridNo, m.ubStrategicInsertionCode);
                if (m.pSoldier->usStrategicInsertionData == NOWHERE) {
                  fInvalidArea = TRUE;
                  break;
                }
              }
            }
            if (!fInvalidArea) {  // One or more of the mercs in the group didn't
                                  // get gridno assignments, so we
              // report an error.
              FOR_EACH_MERC_PLACEMENT(i) {
                MERCPLACEMENT &m = *i;
                m.pSoldier->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
                if (m.pSoldier->ubGroupID == gubSelectedGroupID) {
                  PutDownMercPiece(m);
                }
              }
            }
          } else {  // This is a single merc placement.  If valid, then place
                    // him, else report error.
            MERCPLACEMENT &m = gMercPlacement[gbSelectedMercID];
            m.pSoldier->usStrategicInsertionData =
                SearchForClosestPrimaryMapEdgepoint(sGridNo, m.ubStrategicInsertionCode);
            if (m.pSoldier->usStrategicInsertionData != NOWHERE) {
              m.pSoldier->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
              PutDownMercPiece(m);
            } else {
              fInvalidArea = TRUE;
            }

            // gbSelectedMercID++;
            // if( gbSelectedMercID == giPlacements )
            //	gbSelectedMercID = 0;
            // gpTacticalPlacementSelectedSoldier = gMercPlacement[
            // gbSelectedMercID ].pSoldier;
            gfTacticalPlacementGUIDirty = TRUE;
            // SetCursorMerc( gbSelectedMercID );
          }
          EndMapEdgepointSearch();

          if (fInvalidArea) {  // Report error due to invalid placement.
            SGPBox const CenterRect = {220, 120, 200, 80};
            DoMessageBox(MSG_BOX_BASIC_STYLE, gpStrategicString[STR_TP_INACCESSIBLE_MESSAGE],
                         guiCurrentScreen, MSG_BOX_FLAG_OK, DialogRemoved, &CenterRect);
          } else {  // Placement successful, so select the next unplaced unit
                    // (single or group).
            SelectNextUnplacedUnit();
          }
        }
      }
    } else {  // not a valid cursor location...
      if (gbCursorMercID != -1) {
        SGPBox const CenterRect = {220, 120, 200, 80};
        DoMessageBox(MSG_BOX_BASIC_STYLE, gpStrategicString[STR_TP_INVALID_MESSAGE],
                     guiCurrentScreen, MSG_BOX_FLAG_OK, DialogRemoved, &CenterRect);
      }
    }
  }
}

static void SetCursorMerc(int8_t const placement) {
  if (gbCursorMercID == placement) return;

  if (gbCursorMercID == -1 || placement == -1 ||
      gMercPlacement[gbCursorMercID].ubStrategicInsertionCode !=
          gMercPlacement[placement].ubStrategicInsertionCode) {
    gfValidLocationsChanged = TRUE;
  }
  gbCursorMercID = placement;
}

static void PutDownMercPiece(MERCPLACEMENT &m) {
  SOLDIERTYPE &s = *m.pSoldier;
  GridNo insertion_gridno;
  switch (s.ubStrategicInsertionCode) {
    case INSERTION_CODE_NORTH:
      insertion_gridno = gMapInformation.sNorthGridNo;
      break;
    case INSERTION_CODE_SOUTH:
      insertion_gridno = gMapInformation.sSouthGridNo;
      break;
    case INSERTION_CODE_EAST:
      insertion_gridno = gMapInformation.sEastGridNo;
      break;
    case INSERTION_CODE_WEST:
      insertion_gridno = gMapInformation.sWestGridNo;
      break;
    case INSERTION_CODE_GRIDNO:
      insertion_gridno = s.usStrategicInsertionData;
      break;
    default:
      throw std::logic_error("invalid strategic insertion code");
  }
  s.sInsertionGridNo = insertion_gridno;
  if (m.fPlaced) PickUpMercPiece(m);
  GridNo const gridno = FindGridNoFromSweetSpot(&s, insertion_gridno, 4);
  if (gridno != NOWHERE) {
    EVENT_SetSoldierPositionNoCenter(&s, gridno, SSP_NONE);
    uint8_t const direction = GetDirectionToGridNoFromGridNo(gridno, CENTER_GRIDNO);
    EVENT_SetSoldierDirection(&s, direction);
    s.ubInsertionDirection = s.bDirection;
    m.fPlaced = TRUE;
    m.pSoldier->bInSector = TRUE;
  }
}

static void PickUpMercPiece(MERCPLACEMENT &m) {
  m.fPlaced = FALSE;
  SOLDIERTYPE &s = *m.pSoldier;
  RemoveSoldierFromGridNo(s);
  s.bInSector = FALSE;
}

static void DialogRemoved(MessageBoxReturnValue const ubResult) {
  gfTacticalPlacementGUIDirty = TRUE;
  gfValidLocationsChanged = TRUE;
}
