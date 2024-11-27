#include "Editor/CursorModes.h"

#include <algorithm>

#include "Editor/EditScreen.h"
#include "Editor/EditorBuildings.h"
#include "Editor/EditorDefines.h"
#include "Editor/EditorMercs.h"
#include "Editor/EditorTaskbarUtils.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/Input.h"
#include "SGP/Random.h"
#include "SGP/Types.h"
#include "Tactical/Interface.h"
#include "Tactical/Overhead.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldMan.h"

SGPRect gSelectRegion;

static BOOLEAN fValidCursor = FALSE;
static BOOLEAN fAnchored = FALSE;
static BOOLEAN gfBrushEnabled = TRUE;
uint16_t gusSelectionWidth = 1, gusPreserveSelectionWidth = 1;
uint16_t gusSelectionType = SMALLSELECTION;
uint16_t gusSelectionDensity = 2;
uint16_t gusSavedSelectionType = SMALLSELECTION;
uint16_t gusSavedBuildingSelectionType = AREASELECTION;
static int16_t sBadMarker = -1;

wchar_t SelTypeWidth[10] = L"Width: xx";
const wchar_t *const wszSelType[6] = {L"Small",  L"Medium",    L"Large",
                                      L"XLarge", SelTypeWidth, L"Area"};

static BOOLEAN gfAllowRightButtonSelections = FALSE;
BOOLEAN gfCurrentSelectionWithRightButton = FALSE;

// Used for offseting cursor to show that it is on the roof rather than on the
// ground. This can be conveniently executed by moving the cursor up and right 3
// gridnos for a total of -483  -(160*3)-(1*3)
#define ROOF_OFFSET (-483)
static BOOLEAN gfUsingOffset;

// Based on the density level setting and the selection type, this test will
// randomly choose TRUE or FALSE to reflect the *odds*.
BOOLEAN PerformDensityTest() {
  if (Random(100) < gusSelectionDensity) return TRUE;
  return FALSE;
}

void IncreaseSelectionDensity() {
  switch (gusSelectionDensity) {
    case 2:
      gusSelectionDensity = 5;
      break;
    case 5:
      gusSelectionDensity = 10;
      break;
    default:
      gusSelectionDensity += 10;
      break;
    case 100:
      gusSelectionDensity = 2;
      break;
  }
}

void DecreaseSelectionDensity() {
  switch (gusSelectionDensity) {
    case 2:
      gusSelectionDensity = 100;
      break;
    case 5:
      gusSelectionDensity = 2;
      break;
    case 10:
      gusSelectionDensity = 5;
      break;
    default:
      gusSelectionDensity -= 10;
      break;
  }
}

void RemoveCursors() {
  int32_t x, y, iMapIndex;
  if (gpBuildingLayoutList) {
    RemoveBuildingLayout();
  }
  Assert(gSelectRegion.iTop >= 0 && gSelectRegion.iTop <= gSelectRegion.iBottom);
  Assert(gSelectRegion.iLeft >= 0 && gSelectRegion.iLeft <= gSelectRegion.iRight);
  for (y = gSelectRegion.iTop; y <= gSelectRegion.iBottom; y++) {
    for (x = gSelectRegion.iLeft; x <= gSelectRegion.iRight; x++) {
      LEVELNODE *pNode;
      iMapIndex = y * WORLD_COLS + x;
      if (gfUsingOffset) iMapIndex += ROOF_OFFSET;
      pNode = gpWorldLevelData[iMapIndex].pTopmostHead;
      while (pNode) {
        if (pNode->usIndex == FIRSTPOINTERS1 || pNode->usIndex == FIRSTPOINTERS5) {
          RemoveTopmost(iMapIndex, pNode->usIndex);
          break;
        }
        pNode = pNode->pNext;
      }
    }
  }
  fValidCursor = FALSE;
  gfUsingOffset = FALSE;
}

static void RemoveBadMarker() {
  LEVELNODE *pNode;
  if (sBadMarker < 0) return;
  pNode = gpWorldLevelData[sBadMarker].pTopmostHead;
  while (pNode) {
    if (pNode->usIndex == BADMARKER1) {
      RemoveTopmost(sBadMarker, pNode->usIndex);
      sBadMarker = -1;
      break;
    }
    pNode = pNode->pNext;
  }
}

static void DrawBuildingLayout(int32_t iMapIndex);
static void EnsureSelectionType();
static void ForceAreaSelectionWidth(int16_t sGridX, int16_t sGridY);
static BOOLEAN HandleAreaSelection(int16_t sGridX, int16_t sGridY);
static void ValidateSelectionRegionBoundaries();

void UpdateCursorAreas() {
  int32_t x, y, iMapIndex;

  RemoveCursors();

  EnsureSelectionType();

  // Determine if the mouse is currently in the world.
  int16_t sGridX;
  int16_t sGridY;
  if (gViewportRegion.uiFlags & MSYS_MOUSE_IN_AREA && GetMouseXY(&sGridX, &sGridY)) {
    iMapIndex = MAPROWCOLTOPOS(sGridY, sGridX);
    if (gpBuildingLayoutList) {
      gSelectRegion.iLeft = gSelectRegion.iRight = sGridX;
      gSelectRegion.iTop = gSelectRegion.iBottom = sGridY;
      fValidCursor = TRUE;
      DrawBuildingLayout(iMapIndex);
    } else
      switch (gusSelectionType) {
        case SMALLSELECTION:
          gSelectRegion.iLeft = gSelectRegion.iRight = sGridX;
          gSelectRegion.iTop = gSelectRegion.iBottom = sGridY;
          fValidCursor = TRUE;
          break;
        case MEDIUMSELECTION:
        case LARGESELECTION:
        case XLARGESELECTION:
          // The mouse mode value reflects the size of the cursor.
          gSelectRegion.iTop = sGridY - gusSelectionType;
          gSelectRegion.iBottom = sGridY + gusSelectionType;
          gSelectRegion.iLeft = sGridX - gusSelectionType;
          gSelectRegion.iRight = sGridX + gusSelectionType;
          ValidateSelectionRegionBoundaries();
          fValidCursor = TRUE;
          break;
        case LINESELECTION:
          fValidCursor = HandleAreaSelection(sGridX, sGridY);
          ForceAreaSelectionWidth(sGridX, sGridY);
          ValidateSelectionRegionBoundaries();
          break;
        case AREASELECTION:
          fValidCursor = HandleAreaSelection(sGridX, sGridY);
          break;
      }
  }
  // Draw all of the area cursors here.
  if (fValidCursor) {
    if (iDrawMode == DRAW_MODE_ENEMY || iDrawMode == DRAW_MODE_CREATURE ||
        iDrawMode == DRAW_MODE_REBEL || iDrawMode == DRAW_MODE_CIVILIAN ||
        iDrawMode == DRAW_MODE_SCHEDULEACTION) {
      iMapIndex = gSelectRegion.iTop * WORLD_COLS + gSelectRegion.iLeft;
      if ((!IsLocationSittable(iMapIndex, gfRoofPlacement) &&
           iDrawMode != DRAW_MODE_SCHEDULEACTION) ||
          (!IsLocationSittableExcludingPeople(iMapIndex, gfRoofPlacement) &&
           iDrawMode == DRAW_MODE_SCHEDULEACTION)) {
        if (sBadMarker != iMapIndex) {
          RemoveBadMarker();
          if (gfRoofPlacement && FlatRoofAboveGridNo(iMapIndex)) {
            AddTopmostToTail(iMapIndex + ROOF_OFFSET, BADMARKER1);
            sBadMarker = (int16_t)(iMapIndex + ROOF_OFFSET);
          } else {
            AddTopmostToTail((iMapIndex), BADMARKER1);
            sBadMarker = (int16_t)(iMapIndex);
          }
        }
      } else {
        RemoveBadMarker();
        if (gfRoofPlacement && FlatRoofAboveGridNo(iMapIndex)) {
          AddTopmostToTail(iMapIndex + ROOF_OFFSET, FIRSTPOINTERS5);
          gfUsingOffset = TRUE;
        } else
          AddTopmostToTail(iMapIndex, FIRSTPOINTERS1);
      }
    } else
      for (y = gSelectRegion.iTop; y <= gSelectRegion.iBottom; y++) {
        for (x = gSelectRegion.iLeft; x <= gSelectRegion.iRight; x++) {
          iMapIndex = y * WORLD_COLS + x;
          AddTopmostToTail(iMapIndex, FIRSTPOINTERS1);
        }
      }
  }
}

static SGPPoint gSelectAnchor;

static void ForceAreaSelectionWidth(const int16_t sGridX, const int16_t sGridY) {
  uint16_t gusDecSelWidth;

  // If the anchor isn't set, we don't want to force the size yet.
  if (!fAnchored) return;

  gusDecSelWidth = gusSelectionWidth - 1;

  // compare the region with the anchor and determine if we are going to force
  // size via height or width depending on the cursor distance from the anchor.
  if (abs(sGridX - gSelectAnchor.iX) < abs(sGridY - gSelectAnchor.iY)) {  // restrict the x axis
    if (sGridX < gSelectAnchor.iX) {                                      // to the left
      gSelectRegion.iLeft = gSelectAnchor.iX - gusDecSelWidth;
      gSelectRegion.iRight = gSelectAnchor.iX;
    } else {  // to the right
      gSelectRegion.iLeft = gSelectAnchor.iX;
      gSelectRegion.iRight = gSelectAnchor.iX + gusDecSelWidth;
    }
  } else {                            // restrict the y axis
    if (sGridY < gSelectAnchor.iY) {  // to the upper
      gSelectRegion.iTop = gSelectAnchor.iY - gusDecSelWidth;
      gSelectRegion.iBottom = gSelectAnchor.iY;
    } else {  // to the lower
      gSelectRegion.iBottom = gSelectAnchor.iY + gusDecSelWidth;
      gSelectRegion.iTop = gSelectAnchor.iY;
    }
  }
}

static BOOLEAN HandleAreaSelection(const int16_t sGridX, const int16_t sGridY) {
  // When the user releases the left button, then clear and process the area.
  if (fAnchored) {
    if ((!gfLeftButtonState && !gfCurrentSelectionWithRightButton) ||
        (!gfRightButtonState && gfCurrentSelectionWithRightButton)) {
      fAnchored = FALSE;
      ProcessAreaSelection(!gfCurrentSelectionWithRightButton);
      gfCurrentSelectionWithRightButton = FALSE;
      return FALSE;
    }
  }
  // When the user first clicks, anchor the area.
  if (!fAnchored) {
    if (gfLeftButtonState || (gfRightButtonState && gfAllowRightButtonSelections)) {
      if (gfRightButtonState && !gfLeftButtonState)
        gfCurrentSelectionWithRightButton = TRUE;
      else
        gfCurrentSelectionWithRightButton = FALSE;
      fAnchored = TRUE;
      gSelectAnchor.iX = sGridX;
      gSelectAnchor.iY = sGridY;
      gSelectRegion.iLeft = gSelectRegion.iRight = sGridX;
      gSelectRegion.iTop = gSelectRegion.iBottom = sGridY;
      return TRUE;
    }
  }
  // If no anchoring, then we are really dealing with a single cursor,
  // until the user clicks and holds the mouse button to anchor the cursor.
  if (!fAnchored) {
    gSelectRegion.iLeft = gSelectRegion.iRight = sGridX;
    gSelectRegion.iTop = gSelectRegion.iBottom = sGridY;
    return TRUE;
  }
  // Base the area from the anchor to the current mouse position.
  if (sGridX <= gSelectAnchor.iX) {
    gSelectRegion.iLeft = sGridX;
    gSelectRegion.iRight = gSelectAnchor.iX;
  } else {
    gSelectRegion.iRight = sGridX;
    gSelectRegion.iLeft = gSelectAnchor.iX;
  }
  if (sGridY <= gSelectAnchor.iY) {
    gSelectRegion.iTop = sGridY;
    gSelectRegion.iBottom = gSelectAnchor.iY;
  } else {
    gSelectRegion.iBottom = sGridY;
    gSelectRegion.iTop = gSelectAnchor.iY;
  }
  return TRUE;
}

static void ValidateSelectionRegionBoundaries() {
  gSelectRegion.iLeft = std::max(std::min(159, gSelectRegion.iLeft), 0);
  gSelectRegion.iRight = std::max(std::min(159, gSelectRegion.iRight), 0);
  gSelectRegion.iTop = std::max(std::min(159, gSelectRegion.iTop), 0);
  gSelectRegion.iBottom = std::max(std::min(159, gSelectRegion.iBottom), 0);
}

static void EnsureSelectionType() {
  BOOLEAN fPrevBrushEnabledState = gfBrushEnabled;

  // At time of writing, the only drawing mode supporting right mouse button
  // area selections is the cave drawing mode.
  gfAllowRightButtonSelections = (iDrawMode == DRAW_MODE_CAVES);

  // if we are erasing, we have more flexibility with the drawing modes.
  if (iDrawMode >= DRAW_MODE_ERASE) {
    // erase modes supporting any cursor mode
    gusSavedSelectionType = gusSelectionType;
    gusSelectionWidth = gusPreserveSelectionWidth;
    gfBrushEnabled = TRUE;
  } else
    switch (iDrawMode) {  // regular modes
      case DRAW_MODE_SAW_ROOM:
      case DRAW_MODE_ROOM:
      case DRAW_MODE_CAVES:
        gusSavedBuildingSelectionType = gusSelectionType;
        gusSelectionWidth = gusPreserveSelectionWidth;
        gfBrushEnabled = TRUE;
        break;
      case DRAW_MODE_SLANTED_ROOF:
        gusSelectionType = LINESELECTION;
        gusSelectionWidth = 8;
        gfBrushEnabled = FALSE;
        break;
      case DRAW_MODE_EXITGRID:
      case DRAW_MODE_ROOMNUM:
      case DRAW_MODE_FLOORS:
      case DRAW_MODE_GROUND:
      case DRAW_MODE_OSTRUCTS:
      case DRAW_MODE_OSTRUCTS1:
      case DRAW_MODE_OSTRUCTS2:
      case DRAW_MODE_DEBRIS:
        // supports all modes
        gusSavedSelectionType = gusSelectionType;
        gusSelectionWidth = gusPreserveSelectionWidth;
        gfBrushEnabled = TRUE;
        break;
      default:
        gusSelectionType = SMALLSELECTION;
        gusSelectionWidth = gusPreserveSelectionWidth;
        gfBrushEnabled = FALSE;
        break;
    }

  if (gfBrushEnabled != fPrevBrushEnabledState) {
    if (gfBrushEnabled) {
      EnableEditorButton(TERRAIN_CYCLE_BRUSHSIZE);
      EnableEditorButton(BUILDING_CYCLE_BRUSHSIZE);
      EnableEditorButton(MAPINFO_CYCLE_BRUSHSIZE);
    } else {
      DisableEditorButton(TERRAIN_CYCLE_BRUSHSIZE);
      DisableEditorButton(BUILDING_CYCLE_BRUSHSIZE);
      DisableEditorButton(MAPINFO_CYCLE_BRUSHSIZE);
    }
  }
}

static void DrawBuildingLayout(int32_t iMapIndex) {
  BUILDINGLAYOUTNODE *curr;
  int32_t iOffset;
  LEVELNODE *pNode;
  BOOLEAN fAdd;
  iOffset = iMapIndex - gsBuildingLayoutAnchorGridNo;
  curr = gpBuildingLayoutList;
  while (curr) {
    iMapIndex = curr->sGridNo + iOffset;
    if (iMapIndex > 0 && iMapIndex < WORLD_MAX) {
      fAdd = TRUE;
      pNode = gpWorldLevelData[iMapIndex].pTopmostHead;
      while (pNode) {
        if (pNode->usIndex == FIRSTPOINTERS1) {
          fAdd = FALSE;
          break;
        }
        pNode = pNode->pNext;
      }
      if (fAdd) AddTopmostToTail(iMapIndex, FIRSTPOINTERS1);
    }
    curr = curr->next;
  }
}

void RemoveBuildingLayout() {
  BUILDINGLAYOUTNODE *curr;
  int32_t iOffset;
  int32_t iMapIndex;
  iMapIndex = gSelectRegion.iLeft + gSelectRegion.iTop * WORLD_COLS;
  iOffset = iMapIndex - gsBuildingLayoutAnchorGridNo;
  curr = gpBuildingLayoutList;
  while (curr) {
    iMapIndex = curr->sGridNo + iOffset;
    if (iMapIndex > 0 && iMapIndex < WORLD_MAX) RemoveTopmost(iMapIndex, FIRSTPOINTERS1);
    curr = curr->next;
  }
}
