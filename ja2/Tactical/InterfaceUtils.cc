// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/InterfaceUtils.h"

#include "Directories.h"
#include "JAScreens.h"
#include "Local.h"
#include "SGP/HImage.h"
#include "SGP/Line.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "Tactical/Faces.h"
#include "Tactical/InterfaceControl.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/Vehicles.h"
#include "Tactical/Weapons.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"

#define LIFE_BAR_SHADOW FROMRGB(108, 12, 12)
#define LIFE_BAR FROMRGB(200, 0, 0)
#define BANDAGE_BAR_SHADOW FROMRGB(156, 60, 60)
#define BANDAGE_BAR FROMRGB(222, 132, 132)
#define BLEEDING_BAR_SHADOW FROMRGB(128, 128, 60)
#define BLEEDING_BAR FROMRGB(240, 240, 20)
#define CURR_BREATH_BAR_SHADOW FROMRGB(8, 12, 118)  // the std::max max breatth, always at 100%
#define CURR_BREATH_BAR FROMRGB(8, 12, 160)
#define CURR_MAX_BREATH FROMRGB(0, 0, 0)  // the current max breath, black
#define CURR_MAX_BREATH_SHADOW FROMRGB(0, 0, 0)
#define MORALE_BAR_SHADOW FROMRGB(8, 112, 12)
#define MORALE_BAR FROMRGB(8, 180, 12)
#define BREATH_BAR_SHADOW FROMRGB(60, 108, 108)  // the lt blue current breath
#define BREATH_BAR FROMRGB(113, 178, 218)
#define BREATH_BAR_SHAD_BACK FROMRGB(1, 1, 1)
#define FACE_WIDTH 48
#define FACE_HEIGHT 43

// car portraits
enum {
  ELDORADO_PORTRAIT = 0,
  HUMMER_PORTRAIT,
  ICE_CREAM_TRUCK_PORTRAIT,
  JEEP_PORTRAIT,
  NUMBER_CAR_PORTRAITS
};

// the ids for the car portraits
static SGPVObject *giCarPortraits[NUMBER_CAR_PORTRAITS];

static SGPVObject *guiBrownBackgroundForTeamPanel;  // backgrounds for breath max background

// Load in the portraits for the car faces that will be use in mapscreen
void LoadCarPortraitValues() {
  // the car portrait file names
  static char const *const pbCarPortraitFileNames[] = {
      INTERFACEDIR "/eldorado.sti", INTERFACEDIR "/hummer.sti", INTERFACEDIR "/ice cream truck.sti",
      INTERFACEDIR "/jeep.sti"};

  if (giCarPortraits[0]) return;
  for (int32_t i = 0; i != NUMBER_CAR_PORTRAITS; ++i) {
    giCarPortraits[i] = AddVideoObjectFromFile(pbCarPortraitFileNames[i]);
  }
}

// get rid of the images we loaded for the mapscreen car portraits
void UnLoadCarPortraits() {
  // car protraits loaded?
  if (!giCarPortraits[0]) return;
  for (int32_t i = 0; i != NUMBER_CAR_PORTRAITS; ++i) {
    DeleteVideoObject(giCarPortraits[i]);
    giCarPortraits[i] = 0;
  }
}

static void DrawBar(uint32_t const XPos, uint32_t const YPos, uint32_t const Height,
                    uint16_t const Color, uint16_t const ShadowColor, uint16_t *const DestBuf) {
  LineDraw(TRUE, XPos + 0, YPos, XPos + 0, YPos - Height, ShadowColor, DestBuf);
  LineDraw(TRUE, XPos + 1, YPos, XPos + 1, YPos - Height, Color, DestBuf);
  LineDraw(TRUE, XPos + 2, YPos, XPos + 2, YPos - Height, ShadowColor, DestBuf);
}

static void DrawLifeUIBar(SOLDIERTYPE const &s, uint32_t const XPos, uint32_t YPos,
                          uint32_t const MaxHeight, uint16_t *const pDestBuf) {
  uint32_t Height;

  // FIRST DO std::max LIFE
  Height = MaxHeight * s.bLife / 100;
  DrawBar(XPos, YPos, Height, Get16BPPColor(LIFE_BAR), Get16BPPColor(LIFE_BAR_SHADOW), pDestBuf);

  // NOW DO BANDAGE
  // Calculate bandage
  uint32_t Bandage = s.bLifeMax - s.bLife - s.bBleeding;
  if (Bandage != 0) {
    YPos -= Height;
    Height = MaxHeight * Bandage / 100;
    DrawBar(XPos, YPos, Height, Get16BPPColor(BANDAGE_BAR), Get16BPPColor(BANDAGE_BAR_SHADOW),
            pDestBuf);
  }

  // NOW DO BLEEDING
  if (s.bBleeding != 0) {
    YPos -= Height;
    Height = MaxHeight * s.bBleeding / 100;
    DrawBar(XPos, YPos, Height, Get16BPPColor(BLEEDING_BAR), Get16BPPColor(BLEEDING_BAR_SHADOW),
            pDestBuf);
  }
}

static void DrawBreathUIBar(SOLDIERTYPE const &s, uint32_t const XPos, uint32_t const sYPos,
                            uint32_t const MaxHeight, uint16_t *const pDestBuf) {
  uint32_t Height;

  if (s.bBreathMax <= 97) {
    Height = MaxHeight * (s.bBreathMax + 3) / 100;
    // the old background colors for breath max diff
    DrawBar(XPos, sYPos, Height, Get16BPPColor(BREATH_BAR_SHAD_BACK),
            Get16BPPColor(BREATH_BAR_SHAD_BACK), pDestBuf);
  }

  Height = MaxHeight * s.bBreathMax / 100;
  DrawBar(XPos, sYPos, Height, Get16BPPColor(CURR_MAX_BREATH),
          Get16BPPColor(CURR_MAX_BREATH_SHADOW), pDestBuf);

  // NOW DO BREATH
  Height = MaxHeight * s.bBreath / 100;
  DrawBar(XPos, sYPos, Height, Get16BPPColor(CURR_BREATH_BAR),
          Get16BPPColor(CURR_BREATH_BAR_SHADOW), pDestBuf);
}

static void DrawMoraleUIBar(SOLDIERTYPE const &s, uint32_t const XPos, uint32_t const YPos,
                            uint32_t const MaxHeight, uint16_t *const pDestBuf) {
  uint32_t const Height = MaxHeight * s.bMorale / 100;
  DrawBar(XPos, YPos, Height, Get16BPPColor(MORALE_BAR), Get16BPPColor(MORALE_BAR_SHADOW),
          pDestBuf);
}

void DrawSoldierUIBars(SOLDIERTYPE const &s, int16_t const sXPos, int16_t const sYPos,
                       BOOLEAN const fErase, SGPVSurface *const uiBuffer) {
  const uint32_t BarWidth = 3;
  const uint32_t BarHeight = 42;
  const uint32_t BreathOff = 6;
  const uint32_t MoraleOff = 12;

  // Erase what was there
  if (fErase) {
    RestoreExternBackgroundRect(sXPos, sYPos - BarHeight, MoraleOff + BarWidth, BarHeight + 1);
  }

  if (s.bLife == 0) return;

  if (!(s.uiStatusFlags & SOLDIER_ROBOT)) {
    // DO std::max BREATH
    // brown guy
    uint16_t Region;
    if (guiCurrentScreen != MAP_SCREEN && GetSelectedMan() == &s &&
        gTacticalStatus.ubCurrentTeam == OUR_TEAM && OK_INTERRUPT_MERC(&s)) {
      Region = 1;  // gold, the second entry in the .sti
    } else {
      Region = 0;  // brown, first entry
    }
    BltVideoObject(uiBuffer, guiBrownBackgroundForTeamPanel, Region, sXPos + BreathOff,
                   sYPos - BarHeight);
  }

  SGPVSurface::Lock l(uiBuffer);
  SetClippingRegionAndImageWidth(l.Pitch(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  uint16_t *const pDestBuf = l.Buffer<uint16_t>();

  DrawLifeUIBar(s, sXPos, sYPos, BarHeight, pDestBuf);
  if (!(s.uiStatusFlags & SOLDIER_ROBOT)) {
    DrawBreathUIBar(s, sXPos + BreathOff, sYPos, BarHeight, pDestBuf);
    if (!(s.uiStatusFlags & SOLDIER_VEHICLE)) {
      DrawMoraleUIBar(s, sXPos + MoraleOff, sYPos, BarHeight, pDestBuf);
    }
  }
}

void DrawItemUIBarEx(OBJECTTYPE const &o, const uint8_t ubStatus, const int16_t x, const int16_t y,
                     int16_t max_h, const int16_t sColor1, const int16_t sColor2,
                     SGPVSurface *const uiBuffer) {
  int16_t value;
  // Adjust for ammo, other things
  INVTYPE const &item = Item[o.usItem];
  if (ubStatus >= DRAW_ITEM_STATUS_ATTACHMENT1) {
    value = o.bAttachStatus[ubStatus - DRAW_ITEM_STATUS_ATTACHMENT1];
  } else if (item.usItemClass & IC_AMMO) {
    value = 100 * o.ubShotsLeft[ubStatus] / Magazine[item.ubClassIndex].ubMagSize;
    if (value > 100) value = 100;
  } else if (item.usItemClass & IC_KEY) {
    value = 100;
  } else {
    value = o.bStatus[ubStatus];
  }

  {
    SGPVSurface::Lock l(uiBuffer);
    SetClippingRegionAndImageWidth(l.Pitch(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    uint16_t *const pDestBuf = l.Buffer<uint16_t>();

    --max_h;  // LineDraw() includes the end point
    const int32_t h = max_h * value / 100;
    LineDraw(TRUE, x, y, x, y - h, sColor1, pDestBuf);
    LineDraw(TRUE, x + 1, y, x + 1, y - h, sColor2, pDestBuf);
  }

  if (uiBuffer == guiSAVEBUFFER) {
    RestoreExternBackgroundRect(x, y - max_h, 2, max_h + 1);
  } else {
    InvalidateRegion(x, y - max_h, x + 2, y + 1);
  }
}

void RenderSoldierFace(SOLDIERTYPE const &s, int16_t const sFaceX, int16_t const sFaceY) {
  if (s.uiStatusFlags & SOLDIER_VEHICLE) {
    // just draw the vehicle
    const uint8_t vehicle_type = pVehicleList[s.bVehicleID].ubVehicleType;
    BltVideoObject(guiSAVEBUFFER, giCarPortraits[vehicle_type], 0, sFaceX, sFaceY);
    RestoreExternBackgroundRect(sFaceX, sFaceY, FACE_WIDTH, FACE_HEIGHT);
  } else if (s.face->uiFlags & FACE_INACTIVE_HANDLED_ELSEWHERE)  // OK, check if this face actually
                                                                 // went active
  {
    ExternRenderFace(guiSAVEBUFFER, *s.face, sFaceX, sFaceY);
  } else {
    SetAutoFaceActive(FRAME_BUFFER, guiSAVEBUFFER, *s.face, sFaceX, sFaceY);
    RenderAutoFace(*s.face);
  }
}

void LoadInterfaceUtilsGraphics() {
  guiBrownBackgroundForTeamPanel = AddVideoObjectFromFile(INTERFACEDIR "/bars.sti");
}

void DeleteInterfaceUtilsGraphics() { DeleteVideoObject(guiBrownBackgroundForTeamPanel); }
