#include "Build/Directories.h"
#include "Build/Laptop/Laptop.h"
#include "BobbyRAmmo.h"
#include "Build/Laptop/BobbyRGuns.h"
#include "Build/Laptop/BobbyR.h"
#include "sgp/VObject.h"
#include "Build/Tactical/Interface_Items.h"
#include "Build/Utils/Text.h"
#include "sgp/Button_System.h"
#include "sgp/Video.h"
#include "sgp/VSurface.h"


static SGPVObject* guiAmmoBackground;
static SGPVObject* guiAmmoGrid;


void EnterBobbyRAmmo()
{
	// load the background graphic and add it
	guiAmmoBackground = AddVideoObjectFromFile(LAPTOPDIR "/ammobackground.sti");

	// load the gunsgrid graphic and add it
	guiAmmoGrid = AddVideoObjectFromFile(LAPTOPDIR "/ammogrid.sti");

	InitBobbyBrTitle();


	SetFirstLastPagesForNew( IC_AMMO );

	//Draw menu bar
	InitBobbyMenuBar( );

	RenderBobbyRAmmo( );
}


void ExitBobbyRAmmo()
{
	DeleteVideoObject(guiAmmoBackground);
	DeleteVideoObject(guiAmmoGrid);
	DeleteBobbyMenuBar();

	DeleteBobbyBrTitle();
	DeleteMouseRegionForBigImage();

	giCurrentSubPage = gusCurWeaponIndex;
	guiLastBobbyRayPage = LAPTOP_MODE_BOBBY_R_AMMO;
}


void RenderBobbyRAmmo()
{
	WebPageTileBackground(BOBBYR_NUM_HORIZONTAL_TILES, BOBBYR_NUM_VERTICAL_TILES, BOBBYR_BACKGROUND_WIDTH, BOBBYR_BACKGROUND_HEIGHT, guiAmmoBackground);

	//Display title at top of page
	DisplayBobbyRBrTitle();

	BltVideoObject(FRAME_BUFFER, guiAmmoGrid, 0, BOBBYR_GRIDLOC_X, BOBBYR_GRIDLOC_Y);

	DisplayItemInfo(IC_AMMO);

	UpdateButtonText(guiCurrentLaptopMode);
  MarkButtonsDirty( );
	RenderWWWProgramTitleBar( );
  InvalidateRegion(LAPTOP_SCREEN_UL_X,LAPTOP_SCREEN_WEB_UL_Y,LAPTOP_SCREEN_LR_X,LAPTOP_SCREEN_WEB_LR_Y);
}
