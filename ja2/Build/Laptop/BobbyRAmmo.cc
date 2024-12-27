// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/BobbyRAmmo.h"

#include "Directories.h"
#include "Laptop/BobbyR.h"
#include "Laptop/BobbyRGuns.h"
#include "Laptop/Laptop.h"
#include "SGP/ButtonSystem.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Tactical/InterfaceItems.h"
#include "Utils/Text.h"

static SGPVObject *guiAmmoBackground;
static SGPVObject *guiAmmoGrid;

void EnterBobbyRAmmo() {
  // load the background graphic and add it
  guiAmmoBackground = AddVideoObjectFromFile(LAPTOPDIR "/ammobackground.sti");

  // load the gunsgrid graphic and add it
  guiAmmoGrid = AddVideoObjectFromFile(LAPTOPDIR "/ammogrid.sti");

  InitBobbyBrTitle();

  SetFirstLastPagesForNew(IC_AMMO);

  // Draw menu bar
  InitBobbyMenuBar();

  RenderBobbyRAmmo();
}

void ExitBobbyRAmmo() {
  DeleteVideoObject(guiAmmoBackground);
  DeleteVideoObject(guiAmmoGrid);
  DeleteBobbyMenuBar();

  DeleteBobbyBrTitle();
  DeleteMouseRegionForBigImage();

  giCurrentSubPage = gusCurWeaponIndex;
  guiLastBobbyRayPage = LAPTOP_MODE_BOBBY_R_AMMO;
}

void RenderBobbyRAmmo() {
  WebPageTileBackground(BOBBYR_NUM_HORIZONTAL_TILES, BOBBYR_NUM_VERTICAL_TILES,
                        BOBBYR_BACKGROUND_WIDTH, BOBBYR_BACKGROUND_HEIGHT, guiAmmoBackground);

  // Display title at top of page
  DisplayBobbyRBrTitle();

  BltVideoObject(FRAME_BUFFER, guiAmmoGrid, 0, BOBBYR_GRIDLOC_X, BOBBYR_GRIDLOC_Y);

  DisplayItemInfo(IC_AMMO);

  UpdateButtonText(guiCurrentLaptopMode);
  MarkButtonsDirty();
  RenderWWWProgramTitleBar();
  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}
