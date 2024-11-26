#include "Credits.h"

#include <wchar.h>

#include "Directories.h"
#include "Local.h"
#include "SGP/Debug.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/MemMan.h"
#include "SGP/MouseSystem.h"
#include "SGP/Random.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "Utils/Cursors.h"
#include "Utils/EncryptedFile.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"
#include "Utils/WordWrap.h"

#include "SDL_keycode.h"

struct CRDT_NODE {
  int16_t sPosY;
  int16_t sHeightOfString;  // The height of the displayed string
  SGPVSurface *uiVideoSurfaceImage;
  CRDT_NODE *next;
};

// flags for the credits
#define CRDT_FLAG__TITLE 0x00000001
#define CRDT_FLAG__START_SECTION 0x00000002
#define CRDT_FLAG__END_SECTION 0x00000004

#define CRDT_NAME_OF_CREDIT_FILE BINARYDATADIR "/credits.edt"

#define CREDITS_LINESIZE 80

//
// Code tokens
//
// new codes:
#define CRDT_START_CODE L'@'
#define CRDT_SEPARATION_CODE L','
#define CRDT_END_CODE L';'

#define CRDT_DELAY_BN_STRINGS_CODE 'D'
#define CRDT_DELAY_BN_SECTIONS_CODE 'B'
#define CRDT_SCROLL_SPEED 'S'
#define CRDT_FONT_JUSTIFICATION 'J'
#define CRDT_TITLE_FONT_COLOR 'C'
#define CRDT_ACTIVE_FONT_COLOR 'R'

// Flags:
#define CRDT_TITLE 'T'
#define CRDT_START_OF_SECTION '{'
#define CRDT_END_OF_SECTION '}'

#define CRDT_NAME_LOC_X 375
#define CRDT_NAME_LOC_Y 420
#define CRDT_NAME_TITLE_LOC_Y 435
#define CRDT_NAME_FUNNY_LOC_Y 450
#define CRDT_NAME_LOC_WIDTH 260
#define CRDT_NAME_LOC_HEIGHT \
  (CRDT_NAME_FUNNY_LOC_Y - CRDT_NAME_LOC_Y + GetFontHeight(CRDT_NAME_FONT))

#define CRDT_NAME_FONT FONT12ARIAL

#define CRDT_WIDTH_OF_TEXT_AREA 210
#define CRDT_TEXT_START_LOC 10

#define CRDT_SCROLL_PIXEL_AMOUNT 1
#define CRDT_NODE_DELAY_AMOUNT 25

#define CRDT_SPACE_BN_SECTIONS 50
#define CRDT_SPACE_BN_NODES 12

#define CRDT_START_POS_Y (SCREEN_HEIGHT - 1)

#define CRDT_EYE_WIDTH 30
#define CRDT_EYE_HEIGHT 12

#define CRDT_EYES_CLOSED_TIME 150

struct CreditFace {
  const int16_t sX;
  const int16_t sY;
  const int16_t sWidth;
  const int16_t sHeight;

  const int16_t sEyeX;
  const int16_t sEyeY;

  const int16_t sMouthX;
  const int16_t sMouthY;

  const uint16_t sBlinkFreq;
  uint32_t uiLastBlinkTime;
  uint32_t uiEyesClosedTime;
};

static CreditFace gCreditFaces[] = {
#define M(x, y, w, h, ex, ey, mx, my, f) {x, y, w, h, ex, ey, mx, my, f, 0, 0}
    M(298, 137, 37, 49, 310, 157, 304, 170, 2500),  // Camfield
    M(348, 137, 43, 47, 354, 153, 354, 153, 3700),  // Shawn
    M(407, 132, 30, 50, 408, 151, 410, 164, 3000),  // Kris
    M(443, 131, 30, 50, 447, 151, 446, 161, 4000),  // Ian
    M(487, 136, 43, 50, 493, 155, 493, 155, 3500),  // Linda
    M(529, 145, 43, 50, 536, 164, 536, 164, 4000),  // Eric
    M(581, 132, 43, 48, 584, 150, 583, 161, 3500),  // Lynn
    M(278, 211, 36, 51, 283, 232, 283, 241, 3700),  // Norm
    M(319, 210, 34, 49, 323, 227, 320, 339, 4000),  // George
    M(358, 211, 38, 49, 364, 226, 361, 239, 3600),  // Andrew Stacey
    M(396, 200, 42, 50, 406, 220, 403, 230, 4600),  // Scott
    M(444, 202, 43, 51, 452, 220, 452, 231, 2800),  // Emmons
    M(493, 188, 36, 51, 501, 207, 499, 217, 4500),  // Dave
    M(531, 199, 47, 56, 541, 221, 540, 232, 4000),  // Alex
    M(585, 196, 39, 49, 593, 218, 593, 228, 3500)   // Joey
#undef M
};

static MOUSE_REGION gCrdtMouseRegions[NUM_PEOPLE_IN_CREDITS];

static BOOLEAN g_credits_active;

static SGPVObject *guiCreditBackGroundImage;
static SGPVObject *guiCreditFaces;

static CRDT_NODE *g_credits_head;
static CRDT_NODE *g_credits_tail;

static int32_t giCurrentlySelectedFace;

static Font guiCreditScreenActiveFont;      // the font that is used
static Font guiCreditScreenTitleFont;       // the font that is used
static uint8_t gubCreditScreenActiveColor;  // color of the font
static uint8_t gubCreditScreenTitleColor;   // color of a Title node

static uint32_t guiCrdtNodeScrollSpeed;       // speed credits go up at
static uint32_t guiCrdtLastTimeUpdatingNode;  // the last time a node was read from the file
static uint8_t gubCrdtJustification;          // the current justification

static uint32_t guiGapBetweenCreditSections;
static uint32_t guiGapBetweenCreditNodes;
static uint32_t guiGapTillReadNextCredit;

static uint32_t guiCurrentCreditRecord;
static BOOLEAN gfPauseCreditScreen;

static BOOLEAN EnterCreditsScreen();
static void ExitCreditScreen();
static void GetCreditScreenUserInput();
static void HandleCreditScreen();

ScreenID CreditScreenHandle() {
  if (!g_credits_active) {
    if (!EnterCreditsScreen()) return MAINMENU_SCREEN;
  }

  GetCreditScreenUserInput();
  HandleCreditScreen();
  ExecuteBaseDirtyRectQueue();
  EndFrameBufferRender();

  if (!g_credits_active) {
    ExitCreditScreen();
    return MAINMENU_SCREEN;
  }

  return CREDIT_SCREEN;
}

static void InitCreditEyeBlinking();
static void RenderCreditScreen();
static void SelectCreditFaceMovementRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);

static BOOLEAN EnterCreditsScreen() try {
  guiCreditBackGroundImage = AddVideoObjectFromFile(INTERFACEDIR "/credits.sti");
  guiCreditFaces = AddVideoObjectFromFile(INTERFACEDIR "/credit faces.sti");

  guiCreditScreenActiveFont = FONT12ARIAL;
  gubCreditScreenActiveColor = FONT_MCOLOR_DKWHITE;
  guiCreditScreenTitleFont = FONT14ARIAL;
  gubCreditScreenTitleColor = FONT_MCOLOR_RED;
  guiCrdtNodeScrollSpeed = CRDT_NODE_DELAY_AMOUNT;
  gubCrdtJustification = CENTER_JUSTIFIED;
  guiCurrentCreditRecord = 0;

  guiCrdtLastTimeUpdatingNode = GetJA2Clock();

  guiGapBetweenCreditSections = CRDT_SPACE_BN_SECTIONS;
  guiGapBetweenCreditNodes = CRDT_SPACE_BN_NODES;
  guiGapTillReadNextCredit = CRDT_SPACE_BN_NODES;

  for (int32_t i = 0; i != lengthof(gCrdtMouseRegions); ++i) {
    // Make a mouse region
    MOUSE_REGION *const r = &gCrdtMouseRegions[i];
    CreditFace const &f = gCreditFaces[i];
    uint16_t const x = f.sX;
    uint16_t const y = f.sY;
    uint16_t const w = f.sWidth;
    uint16_t const h = f.sHeight;
    MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_NORMAL, CURSOR_WWW,
                      SelectCreditFaceMovementRegionCallBack, NULL);
    MSYS_SetRegionUserData(r, 0, i);
  }

  giCurrentlySelectedFace = -1;
  gfPauseCreditScreen = FALSE;

  InitCreditEyeBlinking();

  RenderCreditScreen();
  // blit everything to the save buffer (cause the save buffer can bleed
  // through)
  BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);

  g_credits_active = TRUE;
  return TRUE;
} catch (...) {
  return FALSE;
}

static void DeleteFirstNode();

static void ExitCreditScreen() {
  DeleteVideoObject(guiCreditBackGroundImage);
  DeleteVideoObject(guiCreditFaces);

  while (g_credits_head != NULL) DeleteFirstNode();

  for (size_t i = 0; i != lengthof(gCrdtMouseRegions); ++i) {
    MSYS_RemoveRegion(&gCrdtMouseRegions[i]);
  }
}

static BOOLEAN GetNextCreditFromTextFile();
static void HandleCreditEyeBlinking();
static void HandleCreditNodes();

static void HandleCreditScreen() {
  HandleCreditNodes();
  HandleCreditEyeBlinking();

  // is it time to get a new node
  if (g_credits_tail == NULL ||
      (CRDT_START_POS_Y - (g_credits_tail->sPosY + g_credits_tail->sHeightOfString - 16)) >=
          (int16_t)guiGapTillReadNextCredit) {
    // if there are no more credits in the file
    if (!GetNextCreditFromTextFile() && g_credits_tail == NULL) {
      g_credits_active = FALSE;
    }
  }

  RestoreExternBackgroundRect(CRDT_NAME_LOC_X, CRDT_NAME_LOC_Y, CRDT_NAME_LOC_WIDTH,
                              CRDT_NAME_LOC_HEIGHT);

  if (giCurrentlySelectedFace != -1) {
    DrawTextToScreen(gzCreditNames[giCurrentlySelectedFace], CRDT_NAME_LOC_X, CRDT_NAME_LOC_Y,
                     CRDT_NAME_LOC_WIDTH, CRDT_NAME_FONT, FONT_MCOLOR_WHITE, 0,
                     INVALIDATE_TEXT | CENTER_JUSTIFIED);
    DrawTextToScreen(gzCreditNameTitle[giCurrentlySelectedFace], CRDT_NAME_LOC_X,
                     CRDT_NAME_TITLE_LOC_Y, CRDT_NAME_LOC_WIDTH, CRDT_NAME_FONT, FONT_MCOLOR_WHITE,
                     0, INVALIDATE_TEXT | CENTER_JUSTIFIED);
    DrawTextToScreen(gzCreditNameFunny[giCurrentlySelectedFace], CRDT_NAME_LOC_X,
                     CRDT_NAME_FUNNY_LOC_Y, CRDT_NAME_LOC_WIDTH, CRDT_NAME_FONT, FONT_MCOLOR_WHITE,
                     0, INVALIDATE_TEXT | CENTER_JUSTIFIED);
  }
}

static void RenderCreditScreen() {
  BltVideoObject(FRAME_BUFFER, guiCreditBackGroundImage, 0, 0, 0);
  InvalidateScreen();
}

static void GetCreditScreenUserInput() {
  InputAtom Event;
  while (DequeueEvent(&Event)) {
    if (Event.usEvent == KEY_DOWN) {
      switch (Event.usParam) {
        case SDLK_ESCAPE:
          g_credits_active = FALSE;
          break;
      }
    }
  }
}

static void DeleteFirstNode() {
  CRDT_NODE *const del = g_credits_head;

  g_credits_head = del->next;

  if (g_credits_tail == del) g_credits_tail = NULL;

  DeleteVideoSurface(del->uiVideoSurfaceImage);
  MemFree(del);
}

static void AddCreditNode(uint32_t uiFlags, const wchar_t *pString) {
  CRDT_NODE *const pNodeToAdd = MALLOCZ(CRDT_NODE);

  // Determine the font and the color to use
  Font uiFontToUse;
  uint8_t uiColorToUse;
  if (uiFlags & CRDT_FLAG__TITLE) {
    uiFontToUse = guiCreditScreenTitleFont;
    uiColorToUse = gubCreditScreenTitleColor;
  } else {
    uiFontToUse = guiCreditScreenActiveFont;
    uiColorToUse = gubCreditScreenActiveColor;
  }

  // Calculate the height of the string
  pNodeToAdd->sHeightOfString = DisplayWrappedString(0, 0, CRDT_WIDTH_OF_TEXT_AREA, 2, uiFontToUse,
                                                     uiColorToUse, pString, 0, DONT_DISPLAY_TEXT) +
                                1;

  // starting y position on the screen
  pNodeToAdd->sPosY = CRDT_START_POS_Y;

  SGPVSurface *const vs =
      AddVideoSurface(CRDT_WIDTH_OF_TEXT_AREA, pNodeToAdd->sHeightOfString, PIXEL_DEPTH);
  pNodeToAdd->uiVideoSurfaceImage = vs;

  vs->SetTransparency(0);
  vs->Fill(0);

  // write the string onto the surface
  SetFontDestBuffer(vs);
  DisplayWrappedString(0, 1, CRDT_WIDTH_OF_TEXT_AREA, 2, uiFontToUse, uiColorToUse, pString, 0,
                       gubCrdtJustification);
  SetFontDestBuffer(FRAME_BUFFER);

  if (g_credits_tail == NULL) {
    Assert(g_credits_head == NULL);
    g_credits_head = pNodeToAdd;
  } else {
    g_credits_tail->next = pNodeToAdd;
  }
  g_credits_tail = pNodeToAdd;
}

static void DisplayCreditNode(const CRDT_NODE *);

static void HandleCreditNodes() {
  if (g_credits_head == NULL) return;

  if (gfPauseCreditScreen) return;

  if (!(GetJA2Clock() - guiCrdtLastTimeUpdatingNode > guiCrdtNodeScrollSpeed)) return;

  for (CRDT_NODE *i = g_credits_head; i != NULL; i = i->next) {
    DisplayCreditNode(i);
    i->sPosY -= CRDT_SCROLL_PIXEL_AMOUNT;
  }

  const CRDT_NODE *const head = g_credits_head;
  if (head->sPosY + head->sHeightOfString < 0) {
    DeleteFirstNode();
  }

  guiCrdtLastTimeUpdatingNode = GetJA2Clock();
}

static void DisplayCreditNode(const CRDT_NODE *const pCurrent) {
  // Restore the background before blitting the text back on
  int16_t y = pCurrent->sPosY + CRDT_SCROLL_PIXEL_AMOUNT;
  int16_t h = pCurrent->sHeightOfString;
  /* Clip to the screen area */
  if (y < 0) {
    h += y;
    y = 0;
  } else if (y + h > SCREEN_HEIGHT) {
    h = SCREEN_HEIGHT - y;
  }
  RestoreExternBackgroundRect(CRDT_TEXT_START_LOC, y, CRDT_WIDTH_OF_TEXT_AREA, h);

  BltVideoSurface(FRAME_BUFFER, pCurrent->uiVideoSurfaceImage, CRDT_TEXT_START_LOC, pCurrent->sPosY,
                  NULL);
}

static uint32_t GetNumber(const wchar_t *const string) {
  unsigned int v = 0;
  swscanf(string, L"%u", &v);
  return v;
}

static void HandleCreditFlags(uint32_t uiFlags);

static BOOLEAN GetNextCreditFromTextFile() {
  wchar_t text[CREDITS_LINESIZE];
  const uint32_t pos = CREDITS_LINESIZE * guiCurrentCreditRecord++;
  try {
    LoadEncryptedDataFromFile(CRDT_NAME_OF_CREDIT_FILE, text, pos, CREDITS_LINESIZE);
  } catch (...)  // XXX fishy, should check file size beforehand
  {
    return FALSE;
  }

  uint32_t flags = 0;
  const wchar_t *s = text;
  if (*s == CRDT_START_CODE) {
    for (;;) {
      ++s;
      /* process code */
      switch (*s++) {
        case CRDT_DELAY_BN_STRINGS_CODE:
          guiGapBetweenCreditNodes = GetNumber(s);
          break;
        case CRDT_DELAY_BN_SECTIONS_CODE:
          guiGapBetweenCreditSections = GetNumber(s);
          break;
        case CRDT_SCROLL_SPEED:
          guiCrdtNodeScrollSpeed = GetNumber(s);
          break;
        case CRDT_TITLE_FONT_COLOR:
          gubCreditScreenTitleColor = GetNumber(s);
          break;
        case CRDT_ACTIVE_FONT_COLOR:
          gubCreditScreenActiveColor = GetNumber(s);
          break;

        case CRDT_FONT_JUSTIFICATION:
          switch (GetNumber(s)) {
            case 0:
              gubCrdtJustification = LEFT_JUSTIFIED;
              break;
            case 1:
              gubCrdtJustification = CENTER_JUSTIFIED;
              break;
            case 2:
              gubCrdtJustification = RIGHT_JUSTIFIED;
              break;
            default:
              Assert(0);
              break;
          }
          break;

        case CRDT_TITLE:
          flags |= CRDT_FLAG__TITLE;
          break;
        case CRDT_START_OF_SECTION:
          flags |= CRDT_FLAG__START_SECTION;
          break;
        case CRDT_END_OF_SECTION:
          flags |= CRDT_FLAG__END_SECTION;
          break;

        default:
          Assert(0);
          break;
      }

      /* skip till the next code or end of codes */
      while (*s != CRDT_SEPARATION_CODE) {
        switch (*s) {
          case CRDT_END_CODE:
            ++s;
            goto handle_text;
          case L'\0':
            goto handle_text;
          default:
            ++s;
            break;
        }
      }
    }
  }

handle_text:
  if (*s != L'\0') AddCreditNode(flags, s);
  HandleCreditFlags(flags);
  return TRUE;
}

static void HandleCreditFlags(uint32_t uiFlags) {
  if (uiFlags & CRDT_FLAG__START_SECTION) {
    guiGapTillReadNextCredit = guiGapBetweenCreditNodes;
  }

  if (uiFlags & CRDT_FLAG__END_SECTION) {
    guiGapTillReadNextCredit = guiGapBetweenCreditSections;
  }
}

static void SelectCreditFaceMovementRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    giCurrentlySelectedFace = -1;
  } else if (iReason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    giCurrentlySelectedFace = MSYS_GetRegionUserData(pRegion, 0);
  }
}

static void InitCreditEyeBlinking() {
  const uint32_t now = GetJA2Clock();
  FOR_EACH(CreditFace, f, gCreditFaces) { f->uiLastBlinkTime = now + Random(f->sBlinkFreq * 2); }
}

static void HandleCreditEyeBlinking() {
  uint16_t gfx = 0;
  FOR_EACHX(CreditFace, i, gCreditFaces, gfx += 3) {
    CreditFace &f = *i;
    uint32_t const now = GetJA2Clock();
    if (now - f.uiLastBlinkTime > f.sBlinkFreq) {
      int32_t const x = f.sEyeX;
      int32_t const y = f.sEyeY;
      BltVideoObject(FRAME_BUFFER, guiCreditFaces, gfx, x, y);
      InvalidateRegion(x, y, x + CRDT_EYE_WIDTH, y + CRDT_EYE_HEIGHT);

      f.uiLastBlinkTime = now;
      f.uiEyesClosedTime = now + CRDT_EYES_CLOSED_TIME + Random(CRDT_EYES_CLOSED_TIME);
    } else if (now > f.uiEyesClosedTime) {
      RestoreExternBackgroundRect(f.sEyeX, f.sEyeY, CRDT_EYE_WIDTH, CRDT_EYE_HEIGHT);

      f.uiEyesClosedTime = 0;
    }
  }
}

#include "gtest/gtest.h"

TEST(Credits, asserts) { EXPECT_EQ(lengthof(gCreditFaces), NUM_PEOPLE_IN_CREDITS); }
