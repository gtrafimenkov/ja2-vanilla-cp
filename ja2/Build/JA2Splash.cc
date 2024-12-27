// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "JA2Splash.h"

#include "Directories.h"
#include "GameRes.h"
#include "MainMenuScreen.h"
#include "SGP/LibraryDataBase.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/TimerControl.h"

uint32_t guiSplashFrameFade = 10;
uint32_t guiSplashStartTime = 0;

// Simply create videosurface, load image, and draw it to the screen.
void InitJA2SplashScreen() {
  InitializeJA2Clock();

  if (isEnglishVersion()) {
    ClearMainMenu();
  } else {
    const char *const ImageFile = GetMLGFilename(MLG_SPLASH);
    BltVideoSurfaceOnce(FRAME_BUFFER, ImageFile, 0, 0);
  }

  InvalidateScreen();
  RefreshScreen();

  guiSplashStartTime = GetJA2Clock();
}
