// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/BobbyRShipments.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/BobbyR.h"
#include "Laptop/BobbyRGuns.h"
#include "Laptop/BobbyRMailOrder.h"
#include "Laptop/Laptop.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define BOBBYR_SHIPMENT_TITLE_TEXT_FONT FONT14ARIAL
#define BOBBYR_SHIPMENT_TITLE_TEXT_COLOR 157

#define BOBBYR_SHIPMENT_STATIC_TEXT_FONT FONT12ARIAL
#define BOBBYR_SHIPMENT_STATIC_TEXT_COLOR 145

#define BOBBYR_BOBBY_RAY_TITLE_X LAPTOP_SCREEN_UL_X + 171
#define BOBBYR_BOBBY_RAY_TITLE_Y LAPTOP_SCREEN_WEB_UL_Y + 3

#define BOBBYR_ORDER_FORM_TITLE_X BOBBYR_BOBBY_RAY_TITLE_X
#define BOBBYR_ORDER_FORM_TITLE_Y BOBBYR_BOBBY_RAY_TITLE_Y + 37
#define BOBBYR_ORDER_FORM_TITLE_WIDTH 159

#define BOBBYR_SHIPMENT_DELIVERY_GRID_X LAPTOP_SCREEN_UL_X + 2
#define BOBBYR_SHIPMENT_DELIVERY_GRID_Y BOBBYR_SHIPMENT_ORDER_GRID_Y
#define BOBBYR_SHIPMENT_DELIVERY_GRID_WIDTH 183

#define BOBBYR_SHIPMENT_ORDER_GRID_X LAPTOP_SCREEN_UL_X + 223
#define BOBBYR_SHIPMENT_ORDER_GRID_Y LAPTOP_SCREEN_WEB_UL_Y + 62

#define BOBBYR_SHIPMENT_BACK_BUTTON_X 130
#define BOBBYR_SHIPMENT_HOME_BUTTON_X 515
#define BOBBYR_SHIPMENT_BUTTON_Y (400 + LAPTOP_SCREEN_WEB_DELTA_Y + 4)

#define BOBBYR_SHIPMENT_NUM_PREVIOUS_SHIPMENTS 13

#define BOBBYR_SHIPMENT_ORDER_NUM_X 116  // LAPTOP_SCREEN_UL_X + 9
#define BOBBYR_SHIPMENT_ORDER_NUM_START_Y 144
#define BOBBYR_SHIPMENT_ORDER_NUM_WIDTH 64

#define BOBBYR_SHIPMENT_GAP_BTN_LINES 20

#define BOBBYR_SHIPMENT_SHIPMENT_ORDER_NUM_X BOBBYR_SHIPMENT_ORDER_NUM_X
#define BOBBYR_SHIPMENT_SHIPMENT_ORDER_NUM_Y 117

#define BOBBYR_SHIPMENT_NUM_ITEMS_X \
  183  // BOBBYR_SHIPMENT_ORDER_NUM_X+BOBBYR_SHIPMENT_ORDER_NUM_WIDTH+2
#define BOBBYR_SHIPMENT_NUM_ITEMS_Y BOBBYR_SHIPMENT_SHIPMENT_ORDER_NUM_Y
#define BOBBYR_SHIPMENT_NUM_ITEMS_WIDTH 116

static SGPVObject *guiBobbyRShipmentGrid;

static BOOLEAN gfBobbyRShipmentsDirty = FALSE;

static int32_t giBobbyRShipmentSelectedShipment = -1;

// Back Button
static void BtnBobbyRShipmentBackCallback(GUI_BUTTON *btn, int32_t reason);
static BUTTON_PICS *guiBobbyRShipmentBackImage;
static GUIButtonRef guiBobbyRShipmetBack;

// Home Button
static void BtnBobbyRShipmentHomeCallback(GUI_BUTTON *btn, int32_t reason);
static BUTTON_PICS *giBobbyRShipmentHomeImage;
static GUIButtonRef guiBobbyRShipmentHome;

static MOUSE_REGION gSelectedPreviousShipmentsRegion[BOBBYR_SHIPMENT_NUM_PREVIOUS_SHIPMENTS];

static GUIButtonRef MakeButton(BUTTON_PICS *const img, const wchar_t *const text, const int16_t x,
                               const GUI_CALLBACK click) {
  const int16_t shadow_col = BOBBYR_GUNS_SHADOW_COLOR;
  GUIButtonRef const btn =
      CreateIconAndTextButton(img, text, BOBBYR_GUNS_BUTTON_FONT, BOBBYR_GUNS_TEXT_COLOR_ON,
                              shadow_col, BOBBYR_GUNS_TEXT_COLOR_OFF, shadow_col, x,
                              BOBBYR_SHIPMENT_BUTTON_Y, MSYS_PRIORITY_HIGH, click);
  btn->SetCursor(CURSOR_LAPTOP_SCREEN);
  return btn;
}

static void CreatePreviousShipmentsMouseRegions();

void EnterBobbyRShipments() {
  InitBobbyRWoodBackground();

  // load the Order Grid graphic and add it
  guiBobbyRShipmentGrid = AddVideoObjectFromFile(LAPTOPDIR "/bobbyray_onorder.sti");

  guiBobbyRShipmentBackImage = LoadButtonImage(LAPTOPDIR "/cataloguebutton.sti", 0, 1);
  guiBobbyRShipmetBack = MakeButton(guiBobbyRShipmentBackImage, BobbyROrderFormText[BOBBYR_BACK],
                                    BOBBYR_SHIPMENT_BACK_BUTTON_X, BtnBobbyRShipmentBackCallback);

  giBobbyRShipmentHomeImage = UseLoadedButtonImage(guiBobbyRShipmentBackImage, 0, 1);
  guiBobbyRShipmentHome = MakeButton(giBobbyRShipmentHomeImage, BobbyROrderFormText[BOBBYR_HOME],
                                     BOBBYR_SHIPMENT_HOME_BUTTON_X, BtnBobbyRShipmentHomeCallback);

  CreateBobbyRayOrderTitle();

  giBobbyRShipmentSelectedShipment = -1;

  // if there are shipments
  if (giNumberOfNewBobbyRShipment != 0) {
    int32_t iCnt;

    // get the first shipment #
    for (iCnt = 0; iCnt < giNumberOfNewBobbyRShipment; iCnt++) {
      if (gpNewBobbyrShipments[iCnt].fActive) giBobbyRShipmentSelectedShipment = iCnt;
    }
  }

  CreatePreviousShipmentsMouseRegions();
}

static void RemovePreviousShipmentsMouseRegions();

void ExitBobbyRShipments() {
  DeleteBobbyRWoodBackground();
  DestroyBobbyROrderTitle();

  DeleteVideoObject(guiBobbyRShipmentGrid);

  UnloadButtonImage(guiBobbyRShipmentBackImage);
  UnloadButtonImage(giBobbyRShipmentHomeImage);
  RemoveButton(guiBobbyRShipmetBack);
  RemoveButton(guiBobbyRShipmentHome);

  RemovePreviousShipmentsMouseRegions();
}

void HandleBobbyRShipments() {
  if (gfBobbyRShipmentsDirty) {
    gfBobbyRShipmentsDirty = FALSE;

    RenderBobbyRShipments();
  }
}

static void DisplayPreviousShipments();
static void DisplayShipmentGrid();
static void DisplayShipmentTitles();

void RenderBobbyRShipments() {
  //  HVOBJECT hPixHandle;

  DrawBobbyRWoodBackground();

  DrawBobbyROrderTitle();

  // Output the title
  DrawTextToScreen(gzBobbyRShipmentText[BOBBYR_SHIPMENT__TITLE], BOBBYR_ORDER_FORM_TITLE_X,
                   BOBBYR_ORDER_FORM_TITLE_Y, BOBBYR_ORDER_FORM_TITLE_WIDTH,
                   BOBBYR_SHIPMENT_TITLE_TEXT_FONT, BOBBYR_SHIPMENT_TITLE_TEXT_COLOR,
                   FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

  DisplayShipmentGrid();

  if (giBobbyRShipmentSelectedShipment != -1 &&
      gpNewBobbyrShipments[giBobbyRShipmentSelectedShipment].fActive &&
      gpNewBobbyrShipments[giBobbyRShipmentSelectedShipment].fDisplayedInShipmentPage) {
    //		DisplayPurchasedItems( FALSE, BOBBYR_SHIPMENT_ORDER_GRID_X,
    // BOBBYR_SHIPMENT_ORDER_GRID_Y,
    //&LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray[giBobbyRShipmentSelectedShipment].BobbyRayPurchase[0],
    // FALSE );
    DisplayPurchasedItems(
        FALSE, BOBBYR_SHIPMENT_ORDER_GRID_X, BOBBYR_SHIPMENT_ORDER_GRID_Y,
        &gpNewBobbyrShipments[giBobbyRShipmentSelectedShipment].BobbyRayPurchase[0], FALSE,
        giBobbyRShipmentSelectedShipment);
  } else {
    //		DisplayPurchasedItems( FALSE, BOBBYR_SHIPMENT_ORDER_GRID_X,
    // BOBBYR_SHIPMENT_ORDER_GRID_Y,
    //&LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray[giBobbyRShipmentSelectedShipment].BobbyRayPurchase[0],
    // TRUE );
    DisplayPurchasedItems(FALSE, BOBBYR_SHIPMENT_ORDER_GRID_X, BOBBYR_SHIPMENT_ORDER_GRID_Y, NULL,
                          TRUE, giBobbyRShipmentSelectedShipment);
  }

  DisplayShipmentTitles();
  DisplayPreviousShipments();

  MarkButtonsDirty();
  RenderWWWProgramTitleBar();
  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}

static void BtnBobbyRShipmentBackCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    guiCurrentLaptopMode = LAPTOP_MODE_BOBBY_R_MAILORDER;
  }
}

static void BtnBobbyRShipmentHomeCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    guiCurrentLaptopMode = LAPTOP_MODE_BOBBY_R;
  }
}

static void DisplayShipmentGrid() {
  // Shipment Order Grid
  BltVideoObject(FRAME_BUFFER, guiBobbyRShipmentGrid, 0, BOBBYR_SHIPMENT_DELIVERY_GRID_X,
                 BOBBYR_SHIPMENT_DELIVERY_GRID_Y);

  // Order Grid
  BltVideoObject(FRAME_BUFFER, guiBobbyRShipmentGrid, 1, BOBBYR_SHIPMENT_ORDER_GRID_X,
                 BOBBYR_SHIPMENT_ORDER_GRID_Y);
}

static void DisplayShipmentTitles() {
  // output the order #
  DrawTextToScreen(gzBobbyRShipmentText[BOBBYR_SHIPMENT__ORDERED_ON],
                   BOBBYR_SHIPMENT_SHIPMENT_ORDER_NUM_X, BOBBYR_SHIPMENT_SHIPMENT_ORDER_NUM_Y,
                   BOBBYR_SHIPMENT_ORDER_NUM_WIDTH, BOBBYR_SHIPMENT_STATIC_TEXT_FONT,
                   BOBBYR_SHIPMENT_STATIC_TEXT_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

  // Output the # of items
  DrawTextToScreen(gzBobbyRShipmentText[BOBBYR_SHIPMENT__NUM_ITEMS], BOBBYR_SHIPMENT_NUM_ITEMS_X,
                   BOBBYR_SHIPMENT_NUM_ITEMS_Y, BOBBYR_SHIPMENT_NUM_ITEMS_WIDTH,
                   BOBBYR_SHIPMENT_STATIC_TEXT_FONT, BOBBYR_SHIPMENT_STATIC_TEXT_COLOR,
                   FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
}

static int32_t CountNumberValidShipmentForTheShipmentsPage();

static void DisplayPreviousShipments() {
  uint32_t uiCnt;
  wchar_t zText[512];
  uint16_t usPosY = BOBBYR_SHIPMENT_ORDER_NUM_START_Y;
  uint32_t uiNumItems = CountNumberValidShipmentForTheShipmentsPage();
  uint32_t uiNumberItemsInShipments = 0;
  uint32_t uiItemCnt;
  uint8_t ubFontColor = BOBBYR_SHIPMENT_STATIC_TEXT_COLOR;

  // loop through all the shipments
  for (uiCnt = 0; uiCnt < uiNumItems; uiCnt++) {
    // if it is a valid shipment, and can be displayed at bobby r
    if (gpNewBobbyrShipments[uiCnt].fActive &&
        gpNewBobbyrShipments[giBobbyRShipmentSelectedShipment].fDisplayedInShipmentPage) {
      if (uiCnt == (uint32_t)giBobbyRShipmentSelectedShipment) {
        ubFontColor = FONT_MCOLOR_WHITE;
      } else {
        ubFontColor = BOBBYR_SHIPMENT_STATIC_TEXT_COLOR;
      }

      // Display the "ordered on day num"
      swprintf(zText, lengthof(zText), L"%ls %d", gpGameClockString,
               gpNewBobbyrShipments[uiCnt].uiOrderedOnDayNum);
      DrawTextToScreen(zText, BOBBYR_SHIPMENT_ORDER_NUM_X, usPosY, BOBBYR_SHIPMENT_ORDER_NUM_WIDTH,
                       BOBBYR_SHIPMENT_STATIC_TEXT_FONT, ubFontColor, 0, CENTER_JUSTIFIED);

      uiNumberItemsInShipments = 0;

      //		for( uiItemCnt=0;
      // uiItemCnt<LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray[ uiCnt
      //].ubNumberPurchases; uiItemCnt++ )
      for (uiItemCnt = 0; uiItemCnt < gpNewBobbyrShipments[uiCnt].ubNumberPurchases; uiItemCnt++) {
        //			uiNumberItemsInShipments +=
        // LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray[ uiCnt
        //].BobbyRayPurchase[uiItemCnt].ubNumberPurchased;
        uiNumberItemsInShipments +=
            gpNewBobbyrShipments[uiCnt].BobbyRayPurchase[uiItemCnt].ubNumberPurchased;
      }

      // Display the # of items
      swprintf(zText, lengthof(zText), L"%d", uiNumberItemsInShipments);
      DrawTextToScreen(zText, BOBBYR_SHIPMENT_NUM_ITEMS_X, usPosY, BOBBYR_SHIPMENT_NUM_ITEMS_WIDTH,
                       BOBBYR_SHIPMENT_STATIC_TEXT_FONT, ubFontColor, 0, CENTER_JUSTIFIED);
      usPosY += BOBBYR_SHIPMENT_GAP_BTN_LINES;
    }
  }
}

static void SelectPreviousShipmentsRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);

static void CreatePreviousShipmentsMouseRegions() {
  uint32_t uiCnt;
  uint16_t usPosY = BOBBYR_SHIPMENT_ORDER_NUM_START_Y;
  uint16_t usWidth = BOBBYR_SHIPMENT_DELIVERY_GRID_WIDTH;
  uint16_t usHeight = GetFontHeight(BOBBYR_SHIPMENT_STATIC_TEXT_FONT);
  uint32_t uiNumItems = CountNumberOfBobbyPurchasesThatAreInTransit();

  for (uiCnt = 0; uiCnt < uiNumItems; uiCnt++) {
    MSYS_DefineRegion(&gSelectedPreviousShipmentsRegion[uiCnt], BOBBYR_SHIPMENT_ORDER_NUM_X, usPosY,
                      (uint16_t)(BOBBYR_SHIPMENT_ORDER_NUM_X + usWidth),
                      (uint16_t)(usPosY + usHeight), MSYS_PRIORITY_HIGH, CURSOR_WWW,
                      MSYS_NO_CALLBACK, SelectPreviousShipmentsRegionCallBack);
    MSYS_SetRegionUserData(&gSelectedPreviousShipmentsRegion[uiCnt], 0, uiCnt);

    usPosY += BOBBYR_SHIPMENT_GAP_BTN_LINES;
  }
}

static void RemovePreviousShipmentsMouseRegions() {
  uint32_t uiCnt;
  uint32_t uiNumItems = CountNumberOfBobbyPurchasesThatAreInTransit();

  for (uiCnt = 0; uiCnt < uiNumItems; uiCnt++) {
    MSYS_RemoveRegion(&gSelectedPreviousShipmentsRegion[uiCnt]);
  }
}

static void SelectPreviousShipmentsRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    int32_t iSlotID = MSYS_GetRegionUserData(pRegion, 0);

    if (CountNumberOfBobbyPurchasesThatAreInTransit() > iSlotID) {
      int32_t iCnt;
      int32_t iValidShipmentCounter = 0;

      giBobbyRShipmentSelectedShipment = -1;

      // loop through and get the "x" iSlotID shipment
      for (iCnt = 0; iCnt < giNumberOfNewBobbyRShipment; iCnt++) {
        if (gpNewBobbyrShipments[iCnt].fActive) {
          if (iValidShipmentCounter == iSlotID) {
            giBobbyRShipmentSelectedShipment = iCnt;
          }

          iValidShipmentCounter++;
        }
      }
    }

    gfBobbyRShipmentsDirty = TRUE;
  }
}

static int32_t CountNumberValidShipmentForTheShipmentsPage() {
  if (giNumberOfNewBobbyRShipment > BOBBYR_SHIPMENT_NUM_PREVIOUS_SHIPMENTS)
    return (BOBBYR_SHIPMENT_NUM_PREVIOUS_SHIPMENTS);
  else
    return (giNumberOfNewBobbyRShipment);
}
