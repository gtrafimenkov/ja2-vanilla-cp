#include "Build/Directories.h"
#include "Build/Laptop/BobbyR.h"
#include "Build/Laptop/BobbyRGuns.h"
#include "Build/Laptop/BobbyRMisc.h"
#include "Build/Laptop/Laptop.h"
#include "Build/Utils/Text.h"
#include "sgp/Button_System.h"
#include "sgp/Video.h"
#include "sgp/VObject.h"
#include "sgp/VSurface.h"


static SGPVObject* guiMiscBackground;
static SGPVObject* guiMiscGrid;


void EnterBobbyRMisc()
{
	// load the background graphic and add it
	guiMiscBackground = AddVideoObjectFromFile(LAPTOPDIR "/miscbackground.sti");

	// load the gunsgrid graphic and add it
	guiMiscGrid = AddVideoObjectFromFile(LAPTOPDIR "/miscgrid.sti");

	InitBobbyBrTitle();
	//Draw menu bar
	InitBobbyMenuBar( );

	SetFirstLastPagesForNew( IC_BOBBY_MISC );

	RenderBobbyRMisc( );
}


void ExitBobbyRMisc()
{
	DeleteVideoObject(guiMiscBackground);
	DeleteVideoObject(guiMiscGrid);
	DeleteBobbyBrTitle();
	DeleteMouseRegionForBigImage();
	DeleteBobbyMenuBar();

	guiLastBobbyRayPage = LAPTOP_MODE_BOBBY_R_MISC;
}


void RenderBobbyRMisc()
{
	WebPageTileBackground(BOBBYR_NUM_HORIZONTAL_TILES, BOBBYR_NUM_VERTICAL_TILES, BOBBYR_BACKGROUND_WIDTH, BOBBYR_BACKGROUND_HEIGHT, guiMiscBackground);

	//Display title at top of page
	DisplayBobbyRBrTitle();

	BltVideoObject(FRAME_BUFFER, guiMiscGrid, 0, BOBBYR_GRIDLOC_X, BOBBYR_GRIDLOC_Y);

	DisplayItemInfo(IC_BOBBY_MISC);

	UpdateButtonText(guiCurrentLaptopMode);
  MarkButtonsDirty( );
	RenderWWWProgramTitleBar( );
  InvalidateRegion(LAPTOP_SCREEN_UL_X,LAPTOP_SCREEN_WEB_UL_Y,LAPTOP_SCREEN_LR_X,LAPTOP_SCREEN_WEB_LR_Y);
}
