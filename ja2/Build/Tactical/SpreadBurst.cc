#include "Tactical/SpreadBurst.h"

#include "Directories.h"
#include "SGP/CursorControl.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "Tactical/RealTimeInput.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierFind.h"
#include "Tactical/Weapons.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"

#define MAX_BURST_LOCATIONS 50

struct BURST_LOCATIONS {
  int16_t sX;
  int16_t sY;
  int16_t sGridNo;
};

static BURST_LOCATIONS gsBurstLocations[MAX_BURST_LOCATIONS];
static int8_t gbNumBurstLocations = 0;

static SGPVObject *guiBURSTACCUM;

void ResetBurstLocations() { gbNumBurstLocations = 0; }

void AccumulateBurstLocation(int16_t sGridNo) {
  int32_t cnt;

  if (gbNumBurstLocations < MAX_BURST_LOCATIONS) {
    // Check if it already exists!
    for (cnt = 0; cnt < gbNumBurstLocations; cnt++) {
      if (gsBurstLocations[cnt].sGridNo == sGridNo) {
        return;
      }
    }

    gsBurstLocations[gbNumBurstLocations].sGridNo = sGridNo;

    // Get cell X, Y from mouse...
    GetMouseWorldCoords(&(gsBurstLocations[gbNumBurstLocations].sX),
                        &(gsBurstLocations[gbNumBurstLocations].sY));

    gbNumBurstLocations++;
  }
}

void PickBurstLocations(SOLDIERTYPE *pSoldier) {
  uint8_t ubShotsPerBurst;
  float dAccululator = 0;
  float dStep = 0;
  int32_t cnt;
  uint8_t ubLocationNum;

  // OK, using the # of locations, spread them evenly between our current weapon
  // shots per burst value

  // Get shots per burst
  ubShotsPerBurst = Weapon[pSoldier->inv[HANDPOS].usItem].ubShotsPerBurst;

  // Use # gridnos accululated and # burst shots to determine accululator
  dStep = gbNumBurstLocations / (float)ubShotsPerBurst;

  // Loop through our shots!
  for (cnt = 0; cnt < ubShotsPerBurst; cnt++) {
    // Get index into list
    ubLocationNum = (uint8_t)(dAccululator);

    // Add to merc location
    pSoldier->sSpreadLocations[cnt] = gsBurstLocations[ubLocationNum].sGridNo;

    // Acculuate index value
    dAccululator += dStep;
  }

  // OK, they have been added
}

void AIPickBurstLocations(SOLDIERTYPE *pSoldier, int8_t bTargets, SOLDIERTYPE *pTargets[5]) {
  uint8_t ubShotsPerBurst;
  float dAccululator = 0;
  float dStep = 0;
  int32_t cnt;
  uint8_t ubLocationNum;

  // OK, using the # of locations, spread them evenly between our current weapon
  // shots per burst value

  // Get shots per burst
  ubShotsPerBurst = Weapon[pSoldier->inv[HANDPOS].usItem].ubShotsPerBurst;

  // Use # gridnos accululated and # burst shots to determine accululator
  // dStep = gbNumBurstLocations / (float)ubShotsPerBurst;
  // CJC: tweak!
  dStep = bTargets / (float)ubShotsPerBurst;

  // Loop through our shots!
  for (cnt = 0; cnt < ubShotsPerBurst; cnt++) {
    // Get index into list
    ubLocationNum = (uint8_t)(dAccululator);

    // Add to merc location
    pSoldier->sSpreadLocations[cnt] = pTargets[ubLocationNum]->sGridNo;

    // Acculuate index value
    dAccululator += dStep;
  }

  // OK, they have been added
}

void RenderAccumulatedBurstLocations() {
  int32_t cnt;
  int16_t sGridNo;

  if (!gfBeginBurstSpreadTracking) {
    return;
  }

  if (gbNumBurstLocations == 0) {
    return;
  }

  // Loop through each location...

  // If on screen, render

  // Check if it already exists!
  for (cnt = 0; cnt < gbNumBurstLocations; cnt++) {
    sGridNo = gsBurstLocations[cnt].sGridNo;

    if (GridNoOnScreen(sGridNo)) {
      float dOffsetX, dOffsetY;
      float dTempX_S, dTempY_S;
      int16_t sXPos, sYPos;

      dOffsetX = (float)(gsBurstLocations[cnt].sX - gsRenderCenterX);
      dOffsetY = (float)(gsBurstLocations[cnt].sY - gsRenderCenterY);

      // Calculate guy's position
      FloatFromCellToScreenCoordinates(dOffsetX, dOffsetY, &dTempX_S, &dTempY_S);

      sXPos = ((gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2) + (int16_t)dTempX_S;
      sYPos = ((gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2) + (int16_t)dTempY_S -
              gpWorldLevelData[sGridNo].sHeight;

      // Adjust for offset position on screen
      sXPos -= gsRenderWorldOffsetX;
      sYPos -= gsRenderWorldOffsetY;

      // Adjust for render height
      sYPos += gsRenderHeight;

      // sScreenY -= gpWorldLevelData[ sGridNo ].sHeight;

      // Center circle!
      // sXPos -= 10;
      // sYPos -= 10;

      RegisterBackgroundRectSingleFilled(sXPos, sYPos, 40, 40);

      BltVideoObject(FRAME_BUFFER, guiBURSTACCUM, 1, sXPos, sYPos);
    }
  }
}

void LoadSpreadBurstGraphics() {
  guiBURSTACCUM = AddVideoObjectFromFile(INTERFACEDIR "/burst1.sti");
}

void DeleteSpreadBurstGraphics() { DeleteVideoObject(guiBURSTACCUM); }
