#include "Build/Laptop/CharProfile.h"
#include "Build/Directories.h"
#include "sgp/Font.h"
#include "IMP_Attribute_Entrance.h"
#include "Build/Laptop/IMPVideoObjects.h"
#include "Build/Utils/Text.h"
#include "Build/TileEngine/Render_Dirty.h"
#include "Build/Utils/Cursors.h"
#include "Build/Laptop/Laptop.h"
#include "sgp/Button_System.h"
#include "Build/Utils/Font_Control.h"


// the buttons
static BUTTON_PICS* giIMPAttributeEntranceButtonImage[1];
static GUIButtonRef giIMPAttributeEntranceButton[1];


static void BtnIMPAttributeBeginCallback(GUI_BUTTON *btn, INT32 reason);


static void CreateIMPAttributeEntranceButtons(void);


void EnterIMPAttributeEntrance( void )
{

	CreateIMPAttributeEntranceButtons( );
}

void RenderIMPAttributeEntrance( void )
{
   // the background
	RenderProfileBackGround( );

	// avg merc indent
	RenderAvgMercIndentFrame(90, 40 );
}


static void DestroyIMPAttributeEntranceButtons(void);


void ExitIMPAttributeEntrance( void )
{
  // destroy the finish buttons
	DestroyIMPAttributeEntranceButtons( );
}


void HandleIMPAttributeEntrance( void )
{
}


static void CreateIMPAttributeEntranceButtons(void)
{

	// the begin button
	giIMPAttributeEntranceButtonImage[0] = LoadButtonImage(LAPTOPDIR "/button_2.sti", 0, 1);
	 giIMPAttributeEntranceButton[0] = CreateIconAndTextButton( giIMPAttributeEntranceButtonImage[ 0 ], pImpButtonText[ 13 ], FONT12ARIAL,
														 FONT_WHITE, DEFAULT_SHADOW,
														 FONT_WHITE, DEFAULT_SHADOW,
														 LAPTOP_SCREEN_UL_X +  136, LAPTOP_SCREEN_WEB_UL_Y + 314, MSYS_PRIORITY_HIGH,
														 	BtnIMPAttributeBeginCallback);

	giIMPAttributeEntranceButton[0]->SetCursor(CURSOR_WWW);
}


static void DestroyIMPAttributeEntranceButtons(void)
{
	// this function will destroy the buttons needed for the IMP attrib enter page

	// the begin  button
  RemoveButton(giIMPAttributeEntranceButton[ 0 ] );
  UnloadButtonImage(giIMPAttributeEntranceButtonImage[ 0 ] );
}


static void BtnIMPAttributeBeginCallback(GUI_BUTTON *btn, INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		iCurrentImpPage = IMP_ATTRIBUTE_PAGE;
		fButtonPendingFlag = TRUE;
	}
}
