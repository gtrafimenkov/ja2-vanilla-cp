// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/BrokenLink.h"

#include "Laptop/Laptop.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define BROKEN_LINK__FONT FONT12ARIAL
#define BROKEN_LINK__COLOR FONT_MCOLOR_BLACK

#define BROKEN_LINK__MESSAGE_X LAPTOP_SCREEN_UL_X + 20
#define BROKEN_LINK__MESSAGE_Y LAPTOP_SCREEN_UL_Y + 50
#define BROKEN_LINK__MESSAGE_WIDTH (LAPTOP_SCREEN_LR_X - LAPTOP_SCREEN_UL_X)

#define BROKEN_LINK__SITE_NOT_FOUND_Y LAPTOP_SCREEN_UL_Y + 65

void EnterBrokenLink() {
  //	RenderBrokenLink();
}

void ExitBrokenLink() {}

static void DrawBrokenLinkWhiteBackground();

void RenderBrokenLink() {
  // Color fill the laptop white
  DrawBrokenLinkWhiteBackground();

  SetFontShadow(NO_SHADOW);

  // Put up a message saying the link is dead
  DisplayWrappedString(BROKEN_LINK__MESSAGE_X, BROKEN_LINK__MESSAGE_Y, BROKEN_LINK__MESSAGE_WIDTH,
                       2, BROKEN_LINK__FONT, BROKEN_LINK__COLOR,
                       BrokenLinkText[BROKEN_LINK_TXT_ERROR_404], FONT_MCOLOR_BLACK,
                       LEFT_JUSTIFIED);

  // Put up a message saying the link is dead
  DisplayWrappedString(BROKEN_LINK__MESSAGE_X, BROKEN_LINK__SITE_NOT_FOUND_Y,
                       BROKEN_LINK__MESSAGE_WIDTH, 2, BROKEN_LINK__FONT, BROKEN_LINK__COLOR,
                       BrokenLinkText[BROKEN_LINK_TXT_SITE_NOT_FOUND], FONT_MCOLOR_BLACK,
                       LEFT_JUSTIFIED);

  SetFontShadow(DEFAULT_SHADOW);

  InvalidateScreen();
}

static void DrawBrokenLinkWhiteBackground() {
  ColorFillVideoSurfaceArea(FRAME_BUFFER, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
                            LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y,
                            Get16BPPColor(FROMRGB(255, 255, 255)));
}
