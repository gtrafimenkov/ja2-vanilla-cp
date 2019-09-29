#include "Build/AniViewScreen.h"
#include "Build/Credits.h"
#include "Build/Editor/EditScreen.h"
#include "Build/Editor/LoadScreen.h"
#include "Build/Fade_Screen.h"
#include "Build/GameInitOptionsScreen.h"
#include "Build/GameScreen.h"
#include "Build/Intro.h"
#include "Build/JAScreens.h"
#include "Build/Laptop/Laptop.h"
#include "Build/MainMenuScreen.h"
#include "Build/MessageBoxScreen.h"
#include "Build/Options_Screen.h"
#include "Build/SaveLoadScreen.h"
#include "Build/Screens.h"
#include "Build/Strategic/AI_Viewer.h"
#include "Build/Strategic/Auto_Resolve.h"
#include "Build/Strategic/MapScreen.h"
#include "Build/Strategic/Quest_Debug_System.h"
#include "Build/Tactical/ShopKeeper_Interface.h"
#include "Build/Utils/MapUtility.h"


Screens const GameScreens[] =
{
	{ EditScreenInit,       EditScreenHandle,            EditScreenShutdown       },
	{ NULL,                 NULL,                        NULL                     },
	{ NULL,                 NULL,                        NULL                     },
	{ NULL,                 ErrorScreenHandle,           NULL                     }, // Title Screen
	{ NULL,                 InitScreenHandle,            NULL                     }, // Title Screen
	{ MainGameScreenInit,   MainGameScreenHandle,        MainGameScreenShutdown   },
	{ NULL,                 AniEditScreenHandle,         NULL                     },
	{ NULL,                 PalEditScreenHandle,         NULL                     },
	{ NULL,                 DebugScreenHandle,           NULL                     },
	{ MapScreenInit,        MapScreenHandle,             MapScreenShutdown        },
	{ LaptopScreenInit,     LaptopScreenHandle,          LaptopScreenShutdown     },
	{ NULL,                 LoadSaveScreenHandle,        NULL                     },
	{ NULL,                 MapUtilScreenHandle,         NULL                     },
	{ NULL,                 FadeScreenHandle,            NULL                     },
	{ NULL,                 MessageBoxScreenHandle,      MessageBoxScreenShutdown },
	{ NULL,                 MainMenuScreenHandle,        NULL                     },
	{ NULL,                 AutoResolveScreenHandle,     NULL                     },
	{ NULL,                 SaveLoadScreenHandle,        NULL                     },
	{ NULL,                 OptionsScreenHandle,         NULL                     },
	{ ShopKeeperScreenInit, ShopKeeperScreenHandle,      ShopKeeperScreenShutdown },
	{ NULL,                 SexScreenHandle,             NULL                     },
	{ NULL,                 GameInitOptionsScreenHandle, NULL                     },
	{ NULL,                 NULL,                        NULL                     },
	{ NULL,                 IntroScreenHandle,           NULL                     },
	{ NULL,                 CreditScreenHandle,          NULL                     },
	{ QuestDebugScreenInit, QuestDebugScreenHandle,      NULL                     }
};


#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"

TEST(Screens, asserts)
{
  EXPECT_EQ(lengthof(GameScreens), MAX_SCREENS);
}

#endif
