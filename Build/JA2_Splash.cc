#include "Build/Directories.h"
#include "Build/GameRes.h"
#include "Build/JA2_Splash.h"
#include "Build/MainMenuScreen.h"
#include "Build/Utils/Timer_Control.h"
#include "sgp/Video.h"
#include "sgp/VSurface.h"

UINT32 guiSplashFrameFade = 10;
UINT32 guiSplashStartTime = 0;


//Simply create videosurface, load image, and draw it to the screen.
void InitJA2SplashScreen(void)
{
	InitializeJA2Clock();

  if(isEnglishVersion())
  {
    ClearMainMenu();
  }
  else
  {
    const char* const ImageFile = GetMLGFilename(MLG_SPLASH);
    BltVideoSurfaceOnce(FRAME_BUFFER, ImageFile, 0, 0);
  }

	InvalidateScreen();
	RefreshScreen();

	guiSplashStartTime = GetJA2Clock();
}
