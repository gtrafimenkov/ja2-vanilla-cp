#include "Laptop/AIMLinks.h"

#include "Laptop/AIM.h"
#include "Laptop/Laptop.h"
#include "SGP/ButtonSystem.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/MultiLanguageGraphicUtils.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define AIM_LINK_TITLE_FONT FONT14ARIAL
#define AIM_LINK_TITLE_COLOR AIM_GREEN

#define AIM_LINK_NUM_LINKS 3

#define AIM_LINK_LINK_OFFSET_Y 94  // 90

#define AIM_LINK_LINK_WIDTH 420
#define AIM_LINK_LINK_HEIGHT 70

#define AIM_LINK_BOBBY_LINK_X LAPTOP_SCREEN_UL_X + 40
#define AIM_LINK_BOBBY_LINK_Y LAPTOP_SCREEN_WEB_UL_Y + 91

#define AIM_LINK_FUNERAL_LINK_X AIM_LINK_BOBBY_LINK_X
#define AIM_LINK_FUNERAL_LINK_Y AIM_LINK_BOBBY_LINK_Y + AIM_LINK_LINK_OFFSET_Y

#define AIM_LINK_INSURANCE_LINK_X AIM_LINK_BOBBY_LINK_X
#define AIM_LINK_INSURANCE_LINK_Y AIM_LINK_FUNERAL_LINK_Y + AIM_LINK_LINK_OFFSET_Y

#define AIM_LINK_TITLE_X IMAGE_OFFSET_X + 149
#define AIM_LINK_TITLE_Y AIM_SYMBOL_Y + AIM_SYMBOL_SIZE_Y + 10
#define AIM_LINK_TITLE_WIDTH AIM_SYMBOL_WIDTH

static SGPVObject *guiBobbyLink;
static SGPVObject *guiFuneralLink;
static SGPVObject *guiInsuranceLink;
static uint8_t const gubLinkPages[] = {BOBBYR_BOOKMARK, FUNERAL_BOOKMARK, INSURANCE_BOOKMARK};

// Clicking on guys Face
static MOUSE_REGION gSelectedLinkRegion[AIM_LINK_NUM_LINKS];

static void SelectLinkRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);

void EnterAimLinks() {
  InitAimDefaults();
  InitAimMenuBar();

  // Load the Bobby link graphic.
  guiBobbyLink = AddVideoObjectFromFile(GetMLGFilename(MLG_BOBBYRAYLINK));
  // Load the Funeral graphic.
  guiFuneralLink = AddVideoObjectFromFile(GetMLGFilename(MLG_MORTUARYLINK));
  // Load the Insurance graphic.
  guiInsuranceLink = AddVideoObjectFromFile(GetMLGFilename(MLG_INSURANCELINK));

  uint16_t const x = AIM_LINK_LINK_OFFSET_Y;
  uint16_t y = AIM_LINK_BOBBY_LINK_Y;
  uint8_t const *page = gubLinkPages;
  FOR_EACHX(MOUSE_REGION, i, gSelectedLinkRegion, y += AIM_LINK_LINK_OFFSET_Y) {
    MOUSE_REGION &r = *i;
    MSYS_DefineRegion(&r, x, y, x + AIM_LINK_LINK_WIDTH, y + AIM_LINK_LINK_HEIGHT,
                      MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, SelectLinkRegionCallBack);
    MSYS_SetRegionUserData(&r, 0, *page++);
  }

  RenderAimLinks();
}

void ExitAimLinks() {
  RemoveAimDefaults();

  DeleteVideoObject(guiBobbyLink);
  DeleteVideoObject(guiFuneralLink);
  DeleteVideoObject(guiInsuranceLink);

  FOR_EACH(MOUSE_REGION, i, gSelectedLinkRegion) MSYS_RemoveRegion(&*i);

  ExitAimMenuBar();
}

void RenderAimLinks() {
  DrawAimDefaults();
  DisableAimButton();

  BltVideoObject(FRAME_BUFFER, guiBobbyLink, 0, AIM_LINK_BOBBY_LINK_X, AIM_LINK_BOBBY_LINK_Y);
  BltVideoObject(FRAME_BUFFER, guiFuneralLink, 0, AIM_LINK_FUNERAL_LINK_X, AIM_LINK_FUNERAL_LINK_Y);
  BltVideoObject(FRAME_BUFFER, guiInsuranceLink, 0, AIM_LINK_INSURANCE_LINK_X,
                 AIM_LINK_INSURANCE_LINK_Y);

  // Draw Link Title
  DrawTextToScreen(AimLinkText, AIM_LINK_TITLE_X, AIM_LINK_TITLE_Y, AIM_LINK_TITLE_WIDTH,
                   AIM_LINK_TITLE_FONT, AIM_LINK_TITLE_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

  MarkButtonsDirty();
  RenderWWWProgramTitleBar();
  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}

static void SelectLinkRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    uint32_t gNextLaptopPage;

    gNextLaptopPage = MSYS_GetRegionUserData(pRegion, 0);

    GoToWebPage(gNextLaptopPage);
  }
}
