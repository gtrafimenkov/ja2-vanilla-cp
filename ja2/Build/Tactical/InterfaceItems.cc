#include "Tactical/InterfaceItems.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "GameSettings.h"
#include "JAScreens.h"
#include "Laptop/Finances.h"
#include "Laptop/LaptopSave.h"
#include "Local.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/CursorControl.h"
#include "SGP/Debug.h"
#include "SGP/English.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/LoadSaveData.h"
#include "SGP/MemMan.h"
#include "SGP/MouseSystem.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/MapScreenInterfaceMapInventory.h"
#include "Strategic/Quests.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/ArmsDealerInit.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/Faces.h"
#include "Tactical/HandleItems.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceControl.h"
#include "Tactical/InterfaceCursors.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/InterfaceUtils.h"
#include "Tactical/Items.h"
#include "Tactical/Keys.h"
#include "Tactical/LOS.h"
#include "Tactical/LoadSaveObjectType.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/Points.h"
#include "Tactical/ShopKeeperInterface.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/Squads.h"
#include "Tactical/UICursors.h"
#include "Tactical/Weapons.h"
#include "Tactical/WorldItems.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Physics.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TileDef.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"
#include "Utils/TextUtils.h"
#include "Utils/TimerControl.h"
#include "Utils/WordWrap.h"

#define ITEMDESC_FONT BLOCKFONT2
#define ITEMDESC_FONTSHADOW2 32

#define ITEMDESC_FONTAPFORE 218
#define ITEMDESC_FONTHPFORE 24
#define ITEMDESC_FONTBSFORE 125
#define ITEMDESC_FONTHEFORE 75
#define ITEMDESC_FONTHEAPFORE 76

#define ITEMDESC_AMMO_FORE 209

#define ITEMDESC_FONTHIGHLIGHT FONT_MCOLOR_WHITE

#define STATUS_BAR_SHADOW FROMRGB(140, 136, 119)
#define STATUS_BAR FROMRGB(201, 172, 133)
#define DESC_STATUS_BAR_SHADOW STATUS_BAR_SHADOW
#define DESC_STATUS_BAR STATUS_BAR

#define INV_BAR_DX 5
#define INV_BAR_DY 21

#define RENDER_ITEM_NOSTATUS 20
#define RENDER_ITEM_ATTACHMENT1 200

#define ITEM_STATS_WIDTH 26
#define ITEM_STATS_HEIGHT 8
#define MAX_STACK_POPUP_WIDTH 6

#define ITEMDESC_START_X 214
#define ITEMDESC_START_Y 1 + INV_INTERFACE_START_Y
#define ITEMDESC_HEIGHT 133
#define ITEMDESC_WIDTH 320
#define MAP_ITEMDESC_HEIGHT 268
#define MAP_ITEMDESC_WIDTH 272
#define ITEMDESC_ITEM_X (8 + gsInvDescX)
#define ITEMDESC_ITEM_Y (11 + gsInvDescY)

#define CAMO_REGION_HEIGHT 75
#define CAMO_REGION_WIDTH 75

#define BULLET_SING_X (222 + gsInvDescX)
#define BULLET_SING_Y (49 + gsInvDescY)
#define BULLET_BURST_X (263 + gsInvDescX)
#define BULLET_BURST_Y (49 + gsInvDescY)
#define BULLET_WIDTH 3

#define MAP_BULLET_SING_X (77 + gsInvDescX)
#define MAP_BULLET_SING_Y (135 + gsInvDescY)
#define MAP_BULLET_BURST_X (117 + gsInvDescX)
#define MAP_BULLET_BURST_Y (135 + gsInvDescY)

static const SGPBox g_itemdesc_desc_box = {11, 80, 301, 0};
static const SGPBox g_itemdesc_pros_cons_box = {11, 110, 301, 10};
static const SGPBox g_itemdesc_item_status_box = {6, 60, 2, 51};

static const SGPBox g_map_itemdesc_desc_box = {23, 170, 220, 0};
static const SGPBox g_map_itemdesc_pros_cons_box = {23, 230, 220, 10};
static const SGPBox g_map_itemdesc_item_status_box = {18, 54, 2, 42};

#define DOTDOTDOT L"..."
#define COMMA_AND_SPACE L", "

#define ITEM_PROS_AND_CONS(usItem) ((Item[usItem].usItemClass & IC_GUN))

#define ITEMDESC_AMMO_TEXT_X 3
#define ITEMDESC_AMMO_TEXT_Y 2
#define ITEMDESC_AMMO_TEXT_WIDTH 31

#define ITEM_BAR_HEIGHT 20

#define ITEM_FONT TINYFONT1

#define EXCEPTIONAL_DAMAGE 30
#define EXCEPTIONAL_WEIGHT 20
#define EXCEPTIONAL_RANGE 300
#define EXCEPTIONAL_MAGAZINE 30
#define EXCEPTIONAL_AP_COST 7
#define EXCEPTIONAL_BURST_SIZE 5
#define EXCEPTIONAL_RELIABILITY 2
#define EXCEPTIONAL_REPAIR_EASE 2

#define BAD_DAMAGE 23
#define BAD_WEIGHT 45
#define BAD_RANGE 150
#define BAD_MAGAZINE 10
#define BAD_AP_COST 11
#define BAD_RELIABILITY -2
#define BAD_REPAIR_EASE -2

#define KEYRING_X 496
#define KEYRING_Y (INV_INTERFACE_START_Y + 106)
#define MAP_KEYRING_X 217
#define MAP_KEYRING_Y 271
#define KEYRING_WIDTH 29
#define KEYRING_HEIGHT 23
#define TACTICAL_INVENTORY_KEYRING_GRAPHIC_OFFSET_X 215
// enum used for the money buttons
enum {
  M_1000,
  M_100,
  M_10,
  M_DONE,
};

BOOLEAN gfAddingMoneyToMercFromPlayersAccount;

MOUSE_REGION gInvDesc;

OBJECTTYPE *gpItemPointer;
OBJECTTYPE gItemPointer;
BOOLEAN gfItemPointerDifferentThanDefault = FALSE;
SOLDIERTYPE *gpItemPointerSoldier;
int8_t gbItemPointerSrcSlot;
static uint16_t gusItemPointer = 255;
static uint32_t guiNewlyPlacedItemTimer = 0;
static BOOLEAN gfBadThrowItemCTGH;
BOOLEAN gfDontChargeAPsToPickup = FALSE;
static BOOLEAN gbItemPointerLocateGood = FALSE;

// ITEM DESCRIPTION BOX STUFF
static SGPVObject *guiItemDescBox;
static SGPVObject *guiMapItemDescBox;
static SGPVObject *guiItemGraphic;
static SGPVObject *guiMoneyGraphicsForDescBox;
static SGPVObject *guiBullet;
BOOLEAN gfInItemDescBox = FALSE;
static uint32_t guiCurrentItemDescriptionScreen = 0;
OBJECTTYPE *gpItemDescObject = NULL;
static BOOLEAN gfItemDescObjectIsAttachment = FALSE;
static const wchar_t *gzItemName;
static wchar_t gzItemDesc[SIZE_ITEM_INFO];
static wchar_t gzItemPros[SIZE_ITEM_PROS];
static wchar_t gzItemCons[SIZE_ITEM_CONS];
static int16_t gsInvDescX;
static int16_t gsInvDescY;
static uint8_t gubItemDescStatusIndex;
static BUTTON_PICS *giItemDescAmmoButtonImages;
static GUIButtonRef giItemDescAmmoButton;
static SOLDIERTYPE *gpItemDescSoldier;
static BOOLEAN fItemDescDelete = FALSE;
MOUSE_REGION gItemDescAttachmentRegions[4];
static MOUSE_REGION gProsAndConsRegions[2];

static GUIButtonRef guiMoneyButtonBtn[MAX_ATTACHMENTS];
static BUTTON_PICS *guiMoneyButtonImage;
static BUTTON_PICS *guiMoneyDoneButtonImage;

static uint16_t gusOriginalAttachItem[MAX_ATTACHMENTS];
static uint8_t gbOriginalAttachStatus[MAX_ATTACHMENTS];
static SOLDIERTYPE *gpAttachSoldier;

struct MoneyLoc {
  uint16_t x;
  uint16_t y;
};

static const MoneyLoc gMoneyButtonLoc = {343, INV_INTERFACE_START_Y + 11};
static const MoneyLoc gMoneyButtonOffsets[] = {{0, 0}, {34, 0}, {0, 32}, {34, 32}, {8, 22}};
static const MoneyLoc gMapMoneyButtonLoc = {174, 115};

// number of keys on keyring, temp for now
#define NUMBER_KEYS_ON_KEYRING 28
#define KEY_RING_ROW_WIDTH 7
#define MAP_KEY_RING_ROW_WIDTH 4

// ITEM STACK POPUP STUFF
static BOOLEAN gfInItemStackPopup = FALSE;
static SGPVObject *guiItemPopupBoxes;
static OBJECTTYPE *gpItemPopupObject;
static int16_t gsItemPopupX;
static int16_t gsItemPopupY;
static MOUSE_REGION gItemPopupRegions[8];
static MOUSE_REGION gKeyRingRegions[NUMBER_KEYS_ON_KEYRING];
BOOLEAN gfInKeyRingPopup = FALSE;
static uint8_t gubNumItemPopups = 0;
static MOUSE_REGION gItemPopupRegion;
static int16_t gsItemPopupInvX;
static int16_t gsItemPopupInvY;
static int16_t gsItemPopupInvWidth;
static int16_t gsItemPopupInvHeight;

static int16_t gsKeyRingPopupInvX;
static int16_t gsKeyRingPopupInvY;
static int16_t gsKeyRingPopupInvWidth;
static int16_t gsKeyRingPopupInvHeight;

SOLDIERTYPE *gpItemPopupSoldier;

// inventory description done button for mapscreen
GUIButtonRef giMapInvDescButton;

static BOOLEAN gfItemPopupRegionCallbackEndFix = FALSE;

struct INV_DESC_STATS {
  int16_t sX;
  int16_t sY;
  int16_t sValDx;
};

static const SGPBox gMapDescNameBox = {7, 65, 247, 8};
static const SGPBox gDescNameBox = {16, 67, 291, 8};

static const SGPBox g_desc_item_box_map = {23, 10, 124, 48};
static const SGPBox g_desc_item_box = {9, 9, 117, 55};

static const INV_DESC_STATS gWeaponStats[] = {{202, 25, 83}, {202, 15, 83}, {265, 40, 20},
                                              {202, 40, 32}, {202, 50, 32}, {265, 50, 20},
                                              {234, 50, 0},  {290, 50, 0}};

// displayed AFTER the mass/weight/"Kg" line
static const INV_DESC_STATS gMoneyStats[] = {
    {202, 14, 78}, {212, 25, 78}, {202, 40, 78}, {212, 51, 78}};

// displayed AFTER the mass/weight/"Kg" line
static const INV_DESC_STATS gMapMoneyStats[] = {
    {51, 97, 45}, {61, 107, 75}, {51, 125, 45}, {61, 135, 70}};

static const INV_DESC_STATS gMapWeaponStats[] = {
    {72 - 20, 20 + 80 + 8, 86}, {72 - 20, 20 + 80 - 2, 86}, {72 - 20 + 65, 40 + 80 + 4, 21},
    {72 - 20, 40 + 80 + 4, 30}, {72 - 20, 53 + 80 + 2, 30}, {72 - 20 + 65, 53 + 80 + 2, 25},
    {86, 53 + 80 + 2, 0},       {145, 53 + 80 + 2, 0}};

struct AttachmentGfxInfo {
  SGPBox item_box;  // Bounding box of the item relative to a slot
  SGPBox bar_box;   // Bounding box of the status bar relative to a slot
  SGPPoint slot[4];
};

static const AttachmentGfxInfo g_attachment_info = {
    {7, 0, 28, 25}, {2, 2, 2, 22}, {{128, 10}, {162, 10}, {128, 36}, {162, 36}}};

static const AttachmentGfxInfo g_map_attachment_info = {
    {6, 0, 31, 25}, {1, 1, 2, 23}, {{170, 8}, {208, 8}, {170, 34}, {208, 34}}};

static BOOLEAN gfItemDescHelpTextOffset = FALSE;

// A STRUCT USED INTERNALLY FOR INV SLOT REGIONS
struct INV_REGIONS {
  int16_t w;
  int16_t h;
};

// ARRAY FOR INV PANEL INTERFACE ITEM POSITIONS (sX,sY get set via
// InitInvSlotInterface() )
static INV_REGIONS const gSMInvData[] = {
#define M(w, h) {w, h}
    M(HEAD_INV_SLOT_WIDTH, HEAD_INV_SLOT_HEIGHT),  // HELMETPOS
    M(VEST_INV_SLOT_WIDTH, VEST_INV_SLOT_HEIGHT),  // VESTPOS
    M(LEGS_INV_SLOT_WIDTH, LEGS_INV_SLOT_HEIGHT),  // LEGPOS,
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // HEAD1POS
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // HEAD2POS
    M(BIG_INV_SLOT_WIDTH, BIG_INV_SLOT_HEIGHT),    // HANDPOS,
    M(BIG_INV_SLOT_WIDTH, BIG_INV_SLOT_HEIGHT),    // SECONDHANDPOS
    M(BIG_INV_SLOT_WIDTH, BIG_INV_SLOT_HEIGHT),    // BIGPOCK1
    M(BIG_INV_SLOT_WIDTH, BIG_INV_SLOT_HEIGHT),    // BIGPOCK2
    M(BIG_INV_SLOT_WIDTH, BIG_INV_SLOT_HEIGHT),    // BIGPOCK3
    M(BIG_INV_SLOT_WIDTH, BIG_INV_SLOT_HEIGHT),    // BIGPOCK4
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // SMALLPOCK1
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // SMALLPOCK2
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // SMALLPOCK3
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // SMALLPOCK4
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // SMALLPOCK5
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // SMALLPOCK6
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT),      // SMALLPOCK7
    M(SM_INV_SLOT_WIDTH, SM_INV_SLOT_HEIGHT)       // SMALLPOCK8
#undef M
};

struct REMOVE_MONEY {
  uint32_t uiTotalAmount;
  uint32_t uiMoneyRemaining;
  uint32_t uiMoneyRemoving;
};
static REMOVE_MONEY gRemoveMoney;

static MOUSE_REGION gSMInvRegion[NUM_INV_SLOTS];
static MOUSE_REGION gKeyRingPanel;
static MOUSE_REGION gSMInvCamoRegion;
static int8_t gbCompatibleAmmo[NUM_INV_SLOTS];
int8_t gbInvalidPlacementSlot[NUM_INV_SLOTS];
static uint16_t us16BPPItemCyclePlacedItemColors[20];
static SGPVObject *guiBodyInvVO[4][2];
static SGPVObject *guiGoldKeyVO;
int8_t gbCompatibleApplyItem = FALSE;

static SGPVObject *guiMapInvSecondHandBlockout;
static SGPVObject *guiSecItemHiddenVO;
static SGPVObject *guiGUNSM;
static SGPVObject *guiP1ITEMS;
static SGPVObject *guiP2ITEMS;
static SGPVObject *guiP3ITEMS;

static BOOLEAN AttemptToAddSubstring(wchar_t *const zDest, const wchar_t *const zTemp,
                                     uint32_t *const puiStringLength, const uint32_t uiPixLimit) {
  uint32_t uiRequiredStringLength, uiTempStringLength;

  uiTempStringLength = StringPixLength(zTemp, ITEMDESC_FONT);
  uiRequiredStringLength = *puiStringLength + uiTempStringLength;
  if (zDest[0] != 0) {
    uiRequiredStringLength += StringPixLength(COMMA_AND_SPACE, ITEMDESC_FONT);
  }
  if (uiRequiredStringLength < uiPixLimit) {
    if (zDest[0] != 0) {
      wcscat(zDest, COMMA_AND_SPACE);
    }
    wcscat(zDest, zTemp);
    *puiStringLength = uiRequiredStringLength;
    return (TRUE);
  } else {
    wcscat(zDest, DOTDOTDOT);
    return (FALSE);
  }
}

static void GenerateProsString(wchar_t *const zItemPros, OBJECTTYPE const &o,
                               uint32_t const uiPixLimit) {
  uint32_t uiStringLength = 0;
  const wchar_t *zTemp;
  uint16_t usItem = o.usItem;
  uint8_t ubWeight;

  zItemPros[0] = 0;

  ubWeight = Item[usItem].ubWeight;
  if (Item[usItem].usItemClass == IC_GUN) {
    ubWeight += Item[o.usGunAmmoItem].ubWeight;
  }

  if (Item[usItem].ubWeight <= EXCEPTIONAL_WEIGHT) {
    zTemp = g_langRes->Message[STR_LIGHT];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Item[usItem].ubPerPocket >= 1)  // fits in a small pocket
  {
    zTemp = g_langRes->Message[STR_SMALL];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (GunRange(o) >= EXCEPTIONAL_RANGE) {
    zTemp = g_langRes->Message[STR_LONG_RANGE];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Weapon[usItem].ubImpact >= EXCEPTIONAL_DAMAGE) {
    zTemp = g_langRes->Message[STR_HIGH_DAMAGE];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (BaseAPsToShootOrStab(DEFAULT_APS, DEFAULT_AIMSKILL, *gpItemDescObject) <=
      EXCEPTIONAL_AP_COST) {
    zTemp = g_langRes->Message[STR_QUICK_FIRING];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Weapon[usItem].ubShotsPerBurst >= EXCEPTIONAL_BURST_SIZE || usItem == G11) {
    zTemp = g_langRes->Message[STR_FAST_BURST];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Weapon[usItem].ubMagSize > EXCEPTIONAL_MAGAZINE) {
    zTemp = g_langRes->Message[STR_LARGE_AMMO_CAPACITY];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Item[usItem].bReliability >= EXCEPTIONAL_RELIABILITY) {
    zTemp = g_langRes->Message[STR_RELIABLE];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Item[usItem].bRepairEase >= EXCEPTIONAL_REPAIR_EASE) {
    zTemp = g_langRes->Message[STR_EASY_TO_REPAIR];
    if (!AttemptToAddSubstring(zItemPros, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (zItemPros[0] == 0) {
    // empty string, so display "None"
    if (!AttemptToAddSubstring(zItemPros, g_langRes->Message[STR_NONE], &uiStringLength,
                               uiPixLimit)) {
      return;
    }
  }
}

static void GenerateConsString(wchar_t *const zItemCons, OBJECTTYPE const &o,
                               uint32_t const uiPixLimit) {
  uint32_t uiStringLength = 0;
  const wchar_t *zTemp;
  uint8_t ubWeight;
  uint16_t usItem = o.usItem;

  zItemCons[0] = 0;

  // calculate the weight of the item plus ammunition but not including any
  // attachments
  ubWeight = Item[usItem].ubWeight;
  if (Item[usItem].usItemClass == IC_GUN) {
    ubWeight += Item[o.usGunAmmoItem].ubWeight;
  }

  if (ubWeight >= BAD_WEIGHT) {
    zTemp = g_langRes->Message[STR_HEAVY];
    if (!AttemptToAddSubstring(zItemCons, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (GunRange(o) <= BAD_RANGE) {
    zTemp = g_langRes->Message[STR_SHORT_RANGE];
    if (!AttemptToAddSubstring(zItemCons, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Weapon[usItem].ubImpact <= BAD_DAMAGE) {
    zTemp = g_langRes->Message[STR_LOW_DAMAGE];
    if (!AttemptToAddSubstring(zItemCons, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (BaseAPsToShootOrStab(DEFAULT_APS, DEFAULT_AIMSKILL, *gpItemDescObject) >= BAD_AP_COST) {
    zTemp = g_langRes->Message[STR_SLOW_FIRING];
    if (!AttemptToAddSubstring(zItemCons, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Weapon[usItem].ubShotsPerBurst == 0) {
    zTemp = g_langRes->Message[STR_NO_BURST];
    if (!AttemptToAddSubstring(zItemCons, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Weapon[usItem].ubMagSize < BAD_MAGAZINE) {
    zTemp = g_langRes->Message[STR_SMALL_AMMO_CAPACITY];
    if (!AttemptToAddSubstring(zItemCons, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Item[usItem].bReliability <= BAD_RELIABILITY) {
    zTemp = g_langRes->Message[STR_UNRELIABLE];
    if (!AttemptToAddSubstring(zItemCons, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (Item[usItem].bRepairEase <= BAD_REPAIR_EASE) {
    zTemp = g_langRes->Message[STR_HARD_TO_REPAIR];
    if (!AttemptToAddSubstring(zItemCons, zTemp, &uiStringLength, uiPixLimit)) {
      return;
    }
  }

  if (zItemCons[0] == 0) {
    // empty string, so display "None"
    if (!AttemptToAddSubstring(zItemCons, g_langRes->Message[STR_NONE], &uiStringLength,
                               uiPixLimit)) {
      return;
    }
  }
}

void InitInvSlotInterface(INV_REGION_DESC const *const pRegionDesc,
                          INV_REGION_DESC const *const pCamoRegion,
                          MOUSE_CALLBACK const INVMoveCallback,
                          MOUSE_CALLBACK const INVClickCallback,
                          MOUSE_CALLBACK const INVMoveCamoCallback,
                          MOUSE_CALLBACK const INVClickCamoCallback) {
  // Load all four body type images
  guiBodyInvVO[0][0] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_normal_male.sti");
  guiBodyInvVO[0][1] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_normal_male_h.sti");
  guiBodyInvVO[1][0] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_figure_large_male.sti");
  guiBodyInvVO[1][1] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_figure_large_male_h.sti");
  guiBodyInvVO[2][0] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_normal_male.sti");
  guiBodyInvVO[2][1] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_normal_male.sti");
  guiBodyInvVO[3][0] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_figure_female.sti");
  guiBodyInvVO[3][1] = AddVideoObjectFromFile(INTERFACEDIR "/inventory_figure_female_h.sti");

  // Add gold key graphic
  guiGoldKeyVO = AddVideoObjectFromFile(INTERFACEDIR "/gold_key_button.sti");

  // Add camo region
  uint16_t const x = pCamoRegion->sX;
  uint16_t const y = pCamoRegion->sY;
  MSYS_DefineRegion(&gSMInvCamoRegion, x, y, x + CAMO_REGION_WIDTH, y + CAMO_REGION_HEIGHT,
                    MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, INVMoveCamoCallback, INVClickCamoCallback);

  // Add regions for inventory slots
  for (int32_t i = 0; i != NUM_INV_SLOTS;
       ++i) {  // Set inventory pocket coordinates from the table passed in
    int16_t const x = pRegionDesc[i].sX;
    int16_t const y = pRegionDesc[i].sY;
    INV_REGIONS const &r = gSMInvData[i];
    MOUSE_REGION &m = gSMInvRegion[i];
    MSYS_DefineRegion(&m, x, y, x + r.w, y + r.h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR,
                      INVMoveCallback, INVClickCallback);
    MSYS_SetRegionUserData(&m, 0, i);
  }

  memset(gbCompatibleAmmo, 0, sizeof(gbCompatibleAmmo));
}

void InitKeyRingInterface(MOUSE_CALLBACK KeyRingClickCallback) {
  MSYS_DefineRegion(&gKeyRingPanel, KEYRING_X, KEYRING_Y, KEYRING_X + KEYRING_WIDTH,
                    KEYRING_Y + KEYRING_HEIGHT, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR,
                    MSYS_NO_CALLBACK, KeyRingClickCallback);
  gKeyRingPanel.SetFastHelpText(TacticalStr[KEYRING_HELP_TEXT]);
}

void InitMapKeyRingInterface(MOUSE_CALLBACK KeyRingClickCallback) {
  MSYS_DefineRegion(&gKeyRingPanel, MAP_KEYRING_X, MAP_KEYRING_Y, MAP_KEYRING_X + KEYRING_WIDTH,
                    MAP_KEYRING_Y + KEYRING_HEIGHT, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR,
                    MSYS_NO_CALLBACK, KeyRingClickCallback);
  gKeyRingPanel.SetFastHelpText(TacticalStr[KEYRING_HELP_TEXT]);
}

static void EnableKeyRing(BOOLEAN fEnable) {
  if (fEnable) {
    gKeyRingPanel.Enable();
  } else {
    gKeyRingPanel.Disable();
  }
}

void ShutdownKeyRingInterface() { MSYS_RemoveRegion(&gKeyRingPanel); }

void DisableInvRegions(BOOLEAN fDisable) {
  int32_t cnt;

  for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
    if (fDisable) {
      gSMInvRegion[cnt].Disable();
    } else {
      gSMInvRegion[cnt].Enable();
    }
  }

  if (fDisable) {
    gSMInvCamoRegion.Disable();
    gSM_SELMERCMoneyRegion.Disable();
    EnableKeyRing(FALSE);
  } else {
    gSMInvCamoRegion.Enable();
    gSM_SELMERCMoneyRegion.Enable();
    EnableKeyRing(TRUE);
  }
}

void ShutdownInvSlotInterface() {
  // Remove all body type panels
  for (SGPVObject *(*i)[2] = guiBodyInvVO; i != endof(guiBodyInvVO); ++i) {
    FOR_EACH(SGPVObject *, k, *i) DeleteVideoObject(*k);
  }

  DeleteVideoObject(guiGoldKeyVO);

  FOR_EACH(MOUSE_REGION, i, gSMInvRegion) { MSYS_RemoveRegion(&*i); }

  MSYS_RemoveRegion(&gSMInvCamoRegion);
}

void RenderInvBodyPanel(const SOLDIERTYPE *pSoldier, int16_t sX, int16_t sY) {
  // Blit body inv, based on body type
  int8_t bSubImageIndex = gbCompatibleApplyItem;

  BltVideoObject(guiSAVEBUFFER, guiBodyInvVO[pSoldier->ubBodyType][bSubImageIndex], 0, sX, sY);
}

static void INVRenderINVPanelItem(SOLDIERTYPE const &s, int16_t const pocket,
                                  DirtyLevel const dirty_level) {
  guiCurrentItemDescriptionScreen = guiCurrentScreen;
  bool const in_map = guiCurrentScreen == MAP_SCREEN;
  OBJECTTYPE const &o = s.inv[pocket];
  MOUSE_REGION &r = gSMInvRegion[pocket];

  bool hatch_out = false;
  uint16_t outline = SGP_TRANSPARENT;
  if (dirty_level == DIRTYLEVEL2) {
    wchar_t buf[150];
    GetHelpTextForItem(buf, lengthof(buf), o);
    r.SetFastHelpText(buf);

    /* If it's the second hand and this hand cannot contain anything, remove the
     * second hand position graphic */
    if (pocket == SECONDHANDPOS && Item[s.inv[HANDPOS].usItem].fFlags & ITEM_TWO_HANDED) {
      if (in_map) {
        BltVideoObject(guiSAVEBUFFER, guiMapInvSecondHandBlockout, 0, 14, 218);
        RestoreExternBackgroundRect(14, 218, 102, 24);
      } else {
        int32_t const x = 217;
        int32_t const y = INV_INTERFACE_START_Y + 108;
        BltVideoObject(guiSAVEBUFFER, guiSecItemHiddenVO, 0, x, y);
        RestoreExternBackgroundRect(x, y, 72, 28);
      }
    }

    // Check for compatibility with magazines
    if (gbCompatibleAmmo[pocket]) outline = Get16BPPColor(FROMRGB(255, 255, 255));
  }

  int16_t const x = r.X();
  int16_t const y = r.Y();

  // Now render as normal
  DirtyLevel const render_dirty_level =
      s.bNewItemCount[pocket] <= 0 || gsCurInterfacePanel != SM_PANEL ||
              fInterfacePanelDirty == DIRTYLEVEL2
          ? dirty_level
          : DIRTYLEVEL0;  // We have a new item and we are in the right panel
  INVRenderItem(guiSAVEBUFFER, &s, o, x, y, r.W(), r.H(), render_dirty_level, 0, outline);

  if (gbInvalidPlacementSlot[pocket]) {
    if (pocket != SECONDHANDPOS && !gfSMDisableForItems) {  // We are in inv panel and our guy is
                                                            // not = cursor guy
      hatch_out = true;
    }
  } else {
    if (guiCurrentScreen == SHOPKEEPER_SCREEN &&
        ShouldSoldierDisplayHatchOnItem(s.ubProfile, pocket)) {
      hatch_out = true;
    }
  }

  if (hatch_out) {
    SGPVSurface *const dst = in_map ? guiSAVEBUFFER : FRAME_BUFFER;
    DrawHatchOnInventory(dst, x, y, r.W(), r.H());
  }

  if (o.usItem != NOTHING) {  // Add item status bar
    DrawItemUIBarEx(o, 0, x - INV_BAR_DX, y + INV_BAR_DY, ITEM_BAR_HEIGHT,
                    Get16BPPColor(STATUS_BAR), Get16BPPColor(STATUS_BAR_SHADOW), guiSAVEBUFFER);
  }
}

void HandleRenderInvSlots(SOLDIERTYPE const &s, DirtyLevel const dirty_level) {
  if (InItemDescriptionBox() || InItemStackPopup() || InKeyRingPopup()) return;

  for (int32_t i = 0; i != NUM_INV_SLOTS; ++i) {
    INVRenderINVPanelItem(s, i, dirty_level);
  }

  if (KeyExistsInKeyRing(s, ANYKEY)) {
    // blit gold key here?
    int32_t x;
    int32_t y;
    if (guiCurrentItemDescriptionScreen == MAP_SCREEN) {
      x = MAP_KEYRING_X;
      y = MAP_KEYRING_Y;
    } else {
      x = KEYRING_X;
      y = KEYRING_Y;
    }
    BltVideoObject(guiSAVEBUFFER, guiGoldKeyVO, 0, x, y);
    RestoreExternBackgroundRect(x, y, KEYRING_WIDTH, KEYRING_HEIGHT);
  }
}

static BOOLEAN CompatibleAmmoForGun(const OBJECTTYPE *pTryObject, const OBJECTTYPE *pTestObject) {
  if ((Item[pTryObject->usItem].usItemClass & IC_AMMO)) {
    // CHECK
    if (Weapon[pTestObject->usItem].ubCalibre ==
        Magazine[Item[pTryObject->usItem].ubClassIndex].ubCalibre) {
      return (TRUE);
    }
  }
  return (FALSE);
}

static BOOLEAN CompatibleGunForAmmo(const OBJECTTYPE *pTryObject, const OBJECTTYPE *pTestObject) {
  if ((Item[pTryObject->usItem].usItemClass & IC_GUN)) {
    // CHECK
    if (Weapon[pTryObject->usItem].ubCalibre ==
        Magazine[Item[pTestObject->usItem].ubClassIndex].ubCalibre) {
      return (TRUE);
    }
  }
  return (FALSE);
}

static BOOLEAN CompatibleItemForApplyingOnMerc(const OBJECTTYPE *const test) {
  // ATE: If in mapscreen, return false always....
  if (fInMapMode) return FALSE;

  switch (test->usItem) {
    // ATE: Would be nice to have flag here to check for these types....
    case CAMOUFLAGEKIT:
    case ADRENALINE_BOOSTER:
    case REGEN_BOOSTER:
    case SYRINGE_3:
    case SYRINGE_4:
    case SYRINGE_5:
    case ALCOHOL:
    case WINE:
    case BEER:
    case CANTEEN:
    case JAR_ELIXIR:
      return TRUE;

    default:
      return FALSE;
  }
}

static BOOLEAN SoldierContainsAnyCompatibleStuff(const SOLDIERTYPE *const s,
                                                 const OBJECTTYPE *const test) {
  const uint16_t item_class = Item[test->usItem].usItemClass;
  if (item_class & IC_GUN) {
    CFOR_EACH_SOLDIER_INV_SLOT(i, *s) {
      if (CompatibleAmmoForGun(i, test)) return TRUE;
    }
  } else if (item_class & IC_AMMO) {
    CFOR_EACH_SOLDIER_INV_SLOT(i, *s) {
      if (CompatibleGunForAmmo(i, test)) return TRUE;
    }
  }

  // ATE: Put attachment checking here.....

  return FALSE;
}

void HandleAnyMercInSquadHasCompatibleStuff(const OBJECTTYPE *const o) {
  const int32_t squad = CurrentSquad();
  if (squad == NUMBER_OF_SQUADS) return;

  FOR_EACH_IN_SQUAD(i, squad) {
    SOLDIERTYPE const *const s = *i;
    FACETYPE *const f = s->face;
    Assert(f || s->uiStatusFlags & SOLDIER_VEHICLE);
    if (f == NULL) continue;

    if (o == NULL) {
      f->fCompatibleItems = FALSE;
    } else if (SoldierContainsAnyCompatibleStuff(s, o)) {
      f->fCompatibleItems = TRUE;
    }
  }
}

BOOLEAN HandleCompatibleAmmoUIForMapScreen(const SOLDIERTYPE *pSoldier, int32_t bInvPos,
                                           BOOLEAN fOn, BOOLEAN fFromMerc) {
  BOOLEAN fFound = FALSE;
  int32_t cnt;

  const OBJECTTYPE *pTestObject;
  if (!fFromMerc) {
    pTestObject = &(pInventoryPoolList[bInvPos].o);
  } else {
    if (bInvPos == NO_SLOT) {
      pTestObject = NULL;
    } else {
      pTestObject = &(pSoldier->inv[bInvPos]);
    }
  }

  // ATE: If pTest object is NULL, test only for existence of syringes, etc...
  if (pTestObject == NULL) {
    for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
      if (CompatibleItemForApplyingOnMerc(&pSoldier->inv[cnt])) {
        if (fOn != gbCompatibleAmmo[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        gbCompatibleAmmo[cnt] = fOn;
      }
    }

    if (gpItemPointer != NULL) {
      if (CompatibleItemForApplyingOnMerc(gpItemPointer)) {
        // OK, Light up portrait as well.....
        if (fOn) {
          gbCompatibleApplyItem = TRUE;
        } else {
          gbCompatibleApplyItem = FALSE;
        }

        fFound = TRUE;
      }
    }

    if (fFound) {
      fInterfacePanelDirty = DIRTYLEVEL2;
    }

    return (fFound);
  }

  if ((!Item[pTestObject->usItem].fFlags & ITEM_HIDDEN_ADDON)) {
    // First test attachments, which almost any type of item can have....
    for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
      OBJECTTYPE const &o = pSoldier->inv[cnt];

      if (Item[o.usItem].fFlags & ITEM_HIDDEN_ADDON) {
        // don't consider for UI purposes
        continue;
      }

      uint16_t const a = o.usItem;
      uint16_t const b = pTestObject->usItem;
      if (ValidAttachment(a, b) || ValidAttachment(b, a) || ValidLaunchable(b, a) ||
          ValidLaunchable(a, b)) {
        if (fOn != gbCompatibleAmmo[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        gbCompatibleAmmo[cnt] = fOn;
      }
    }
  }

  if ((Item[pTestObject->usItem].usItemClass & IC_GUN)) {
    for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
      if (CompatibleAmmoForGun(&pSoldier->inv[cnt], pTestObject)) {
        if (fOn != gbCompatibleAmmo[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        gbCompatibleAmmo[cnt] = fOn;
      }
    }
  } else if ((Item[pTestObject->usItem].usItemClass & IC_AMMO)) {
    for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
      if (CompatibleGunForAmmo(&pSoldier->inv[cnt], pTestObject)) {
        if (fOn != gbCompatibleAmmo[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        gbCompatibleAmmo[cnt] = fOn;
      }
    }
  }

  return (fFound);
}

BOOLEAN HandleCompatibleAmmoUIForMapInventory(SOLDIERTYPE *pSoldier, int32_t bInvPos,
                                              int32_t iStartSlotNumber, BOOLEAN fOn,
                                              BOOLEAN fFromMerc) {
  // CJC: ATE, needs fixing here!

  BOOLEAN fFound = FALSE;
  int32_t cnt;
  OBJECTTYPE *pObject, *pTestObject;

  if (!fFromMerc) {
    pTestObject = &(pInventoryPoolList[iStartSlotNumber + bInvPos].o);
  } else {
    if (bInvPos == NO_SLOT) {
      pTestObject = NULL;
    } else {
      pTestObject = &(pSoldier->inv[bInvPos]);
    }
  }

  // First test attachments, which almost any type of item can have....
  for (cnt = 0; cnt < MAP_INVENTORY_POOL_SLOT_COUNT; cnt++) {
    pObject = &(pInventoryPoolList[iStartSlotNumber + cnt].o);

    if (Item[pObject->usItem].fFlags & ITEM_HIDDEN_ADDON) {
      // don't consider for UI purposes
      continue;
    }

    if (ValidAttachment(pObject->usItem, pTestObject->usItem) ||
        ValidAttachment(pTestObject->usItem, pObject->usItem) ||
        ValidLaunchable(pTestObject->usItem, pObject->usItem) ||
        ValidLaunchable(pObject->usItem, pTestObject->usItem)) {
      if (fOn != fMapInventoryItemCompatable[cnt]) {
        fFound = TRUE;
      }

      // IT's an OK calibere ammo, do something!
      // Render Item with specific color
      fMapInventoryItemCompatable[cnt] = fOn;
    }
  }

  if ((Item[pTestObject->usItem].usItemClass & IC_GUN)) {
    for (cnt = 0; cnt < MAP_INVENTORY_POOL_SLOT_COUNT; cnt++) {
      pObject = &(pInventoryPoolList[iStartSlotNumber + cnt].o);

      if (CompatibleAmmoForGun(pObject, pTestObject)) {
        if (fOn != fMapInventoryItemCompatable[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        fMapInventoryItemCompatable[cnt] = fOn;
      }
    }
  } else if ((Item[pTestObject->usItem].usItemClass & IC_AMMO)) {
    for (cnt = 0; cnt < MAP_INVENTORY_POOL_SLOT_COUNT; cnt++) {
      pObject = &(pInventoryPoolList[iStartSlotNumber + cnt].o);

      if (CompatibleGunForAmmo(pObject, pTestObject)) {
        if (fOn != fMapInventoryItemCompatable[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        fMapInventoryItemCompatable[cnt] = fOn;
      }
    }
  }

  return (fFound);
}

BOOLEAN InternalHandleCompatibleAmmoUI(const SOLDIERTYPE *pSoldier, const OBJECTTYPE *pTestObject,
                                       BOOLEAN fOn) {
  BOOLEAN fFound = FALSE;
  int32_t cnt;
  BOOLEAN fFoundAttachment = FALSE;

  // ATE: If pTest object is NULL, test only for existence of syringes, etc...
  if (pTestObject == NULL) {
    for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
      if (CompatibleItemForApplyingOnMerc(&pSoldier->inv[cnt])) {
        if (fOn != gbCompatibleAmmo[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        gbCompatibleAmmo[cnt] = fOn;
      }
    }

    if (gpItemPointer != NULL) {
      if (CompatibleItemForApplyingOnMerc(gpItemPointer)) {
        // OK, Light up portrait as well.....
        if (fOn) {
          gbCompatibleApplyItem = TRUE;
        } else {
          gbCompatibleApplyItem = FALSE;
        }

        fFound = TRUE;
      }
    }

    if (fFound) {
      fInterfacePanelDirty = DIRTYLEVEL2;
    }

    return (fFound);
  }

  // First test attachments, which almost any type of item can have....
  for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
    OBJECTTYPE const &o = pSoldier->inv[cnt];

    if (Item[o.usItem].fFlags & ITEM_HIDDEN_ADDON) {
      // don't consider for UI purposes
      continue;
    }

    uint16_t const a = o.usItem;
    uint16_t const b = pTestObject->usItem;
    if (ValidAttachment(a, b) || ValidAttachment(b, a) || ValidLaunchable(b, a) ||
        ValidLaunchable(a, b)) {
      fFoundAttachment = TRUE;

      if (fOn != gbCompatibleAmmo[cnt]) {
        fFound = TRUE;
      }

      // IT's an OK calibere ammo, do something!
      // Render Item with specific color
      gbCompatibleAmmo[cnt] = fOn;
    }
  }

  // if ( !fFoundAttachment )
  //{
  if ((Item[pTestObject->usItem].usItemClass & IC_GUN)) {
    for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
      if (CompatibleAmmoForGun(&pSoldier->inv[cnt], pTestObject)) {
        if (fOn != gbCompatibleAmmo[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        gbCompatibleAmmo[cnt] = fOn;
      }
    }
  }

  else if ((Item[pTestObject->usItem].usItemClass & IC_AMMO)) {
    for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
      if (CompatibleGunForAmmo(&pSoldier->inv[cnt], pTestObject)) {
        if (fOn != gbCompatibleAmmo[cnt]) {
          fFound = TRUE;
        }

        // IT's an OK calibere ammo, do something!
        // Render Item with specific color
        gbCompatibleAmmo[cnt] = fOn;
      }
    }
  } else if (CompatibleItemForApplyingOnMerc(pTestObject)) {
    // If we are currently NOT in the Shopkeeper interface
    if (guiCurrentScreen != SHOPKEEPER_SCREEN) {
      fFound = TRUE;
      gbCompatibleApplyItem = fOn;
    }
  }
  //}

  if (!fFound) {
    for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
      if (gbCompatibleAmmo[cnt]) {
        fFound = TRUE;
        gbCompatibleAmmo[cnt] = FALSE;
      }

      if (gbCompatibleApplyItem) {
        fFound = TRUE;
        gbCompatibleApplyItem = FALSE;
      }
    }
  }

  if (fFound) {
    fInterfacePanelDirty = DIRTYLEVEL2;
  }

  return (fFound);
}

void ResetCompatibleItemArray() { FOR_EACH(int8_t, i, gbCompatibleAmmo) *i = FALSE; }

BOOLEAN HandleCompatibleAmmoUI(const SOLDIERTYPE *pSoldier, int8_t bInvPos, BOOLEAN fOn) {
  int32_t cnt;

  // if we are in the shopkeeper interface
  const OBJECTTYPE *pTestObject;
  if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
    // if the inventory position is -1, this is a flag from the Shopkeeper
    // interface screen
    // indicating that we are to use a different object to do the search
    if (bInvPos == -1) {
      if (fOn) {
        if (gpHighLightedItemObject) {
          pTestObject = gpHighLightedItemObject;
          //					gubSkiDirtyLevel =
          // SKI_DIRTY_LEVEL2;
        } else
          return (FALSE);
      } else {
        gpHighLightedItemObject = NULL;

        for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
          if (gbCompatibleAmmo[cnt]) {
            gbCompatibleAmmo[cnt] = FALSE;
          }
        }

        gubSkiDirtyLevel = SKI_DIRTY_LEVEL1;
        return (TRUE);
      }
    } else {
      if (fOn) {
        pTestObject = &(pSoldier->inv[bInvPos]);
        gpHighLightedItemObject = pTestObject;
      } else {
        pTestObject = &(pSoldier->inv[bInvPos]);
        gpHighLightedItemObject = NULL;
        gubSkiDirtyLevel = SKI_DIRTY_LEVEL1;
      }
    }
  } else {
    //		if( fOn )

    if (bInvPos == NO_SLOT) {
      pTestObject = NULL;
    } else {
      pTestObject = &(pSoldier->inv[bInvPos]);
    }
  }

  return (InternalHandleCompatibleAmmoUI(pSoldier, pTestObject, fOn));
}

void HandleNewlyAddedItems(SOLDIERTYPE &s, DirtyLevel *const dirty_level) {
  // If item description up, stop
  if (gfInItemDescBox) return;

  for (uint32_t i = 0; i != NUM_INV_SLOTS; ++i) {
    int8_t &new_item_count = s.bNewItemCount[i];
    if (new_item_count == -2) {  // Stop
      *dirty_level = DIRTYLEVEL2;
      new_item_count = 0;
    }

    if (new_item_count <= 0) continue;
    OBJECTTYPE const &o = s.inv[i];
    if (o.usItem == NOTHING) continue;
    MOUSE_REGION const &r = gSMInvRegion[i];
    uint16_t const colour = us16BPPItemCyclePlacedItemColors[s.bNewItemCycleCount[i]];
    INVRenderItem(guiSAVEBUFFER, &s, o, r.X(), r.Y(), r.W(), r.H(), DIRTYLEVEL2, 0, colour);
  }
}

void CheckForAnyNewlyAddedItems(SOLDIERTYPE *pSoldier) {
  uint32_t cnt;

  // OK, l0ok for any new...
  for (cnt = 0; cnt < NUM_INV_SLOTS; cnt++) {
    if (pSoldier->bNewItemCount[cnt] == -1) {
      pSoldier->bNewItemCount[cnt] = NEW_ITEM_CYCLES - 1;
    }
  }
}

void DegradeNewlyAddedItems() {
  // If time done
  const uint32_t uiTime = GetJA2Clock();
  if (uiTime - guiNewlyPlacedItemTimer <= 100) return;

  guiNewlyPlacedItemTimer = uiTime;

  for (uint32_t cnt2 = 0; cnt2 < NUM_TEAM_SLOTS; ++cnt2) {
    SOLDIERTYPE *const s = GetPlayerFromInterfaceTeamSlot(cnt2);
    if (s == NULL) continue;

    for (uint32_t cnt = 0; cnt < NUM_INV_SLOTS; ++cnt) {
      if (s->bNewItemCount[cnt] == 0) continue;

      // Decrement all the time!
      s->bNewItemCycleCount[cnt]--;
      if (s->bNewItemCycleCount[cnt] != 0) continue;

      // OK, cycle down....
      s->bNewItemCount[cnt]--;
      if (s->bNewItemCount[cnt] == 0) {
        // Stop...
        s->bNewItemCount[cnt] = -2;
      } else {
        // Reset!
        s->bNewItemCycleCount[cnt] = NEW_ITEM_CYCLE_COUNT;
      }
    }
  }
}

void INVRenderItem(SGPVSurface *const buffer, SOLDIERTYPE const *const s, OBJECTTYPE const &o,
                   int16_t const sX, int16_t const sY, int16_t const sWidth, int16_t const sHeight,
                   DirtyLevel const dirty_level, uint8_t const ubStatusIndex,
                   int16_t const outline_colour) {
  if (o.usItem == NOTHING) return;
  if (dirty_level == DIRTYLEVEL0) return;

  INVTYPE const &item = ubStatusIndex < RENDER_ITEM_ATTACHMENT1
                            ? Item[o.usItem]
                            : Item[o.usAttachItem[ubStatusIndex - RENDER_ITEM_ATTACHMENT1]];

  if (dirty_level == DIRTYLEVEL2) {
    // Center the object in the slot
    SGPVObject const &item_vo = GetInterfaceGraphicForItem(item);
    uint8_t const gfx_idx = item.ubGraphicNum;
    ETRLEObject const &e = item_vo.SubregionProperties(gfx_idx);
    int16_t const cx = sX + (sWidth - e.usWidth) / 2 - e.sOffsetX;
    int16_t const cy = sY + (sHeight - e.usHeight) / 2 - e.sOffsetY;

    BltVideoObjectOutlineShadow(buffer, &item_vo, gfx_idx, cx - 2, cy + 2);
    BltVideoObjectOutline(buffer, &item_vo, gfx_idx, cx, cy, outline_colour);

    if (buffer == FRAME_BUFFER) {
      InvalidateRegion(sX, sY, sX + sWidth, sY + sHeight);
    } else {
      RestoreExternBackgroundRect(sX, sY, sWidth, sHeight);
    }
  }

  if (ubStatusIndex < RENDER_ITEM_ATTACHMENT1) {
    SetFont(ITEM_FONT);
    SetFontBackground(FONT_MCOLOR_BLACK);

    if (item.usItemClass == IC_GUN && o.usItem != ROCKET_LAUNCHER) {
      // Display free rounds remianing
      uint8_t colour;
      switch (o.ubGunAmmoType) {
        case AMMO_AP:
        case AMMO_SUPER_AP:
          colour = ITEMDESC_FONTAPFORE;
          break;
        case AMMO_HP:
          colour = ITEMDESC_FONTHPFORE;
          break;
        case AMMO_BUCKSHOT:
          colour = ITEMDESC_FONTBSFORE;
          break;
        case AMMO_HE:
          colour = ITEMDESC_FONTHEFORE;
          break;
        case AMMO_HEAT:
          colour = ITEMDESC_FONTHEAPFORE;
          break;
        default:
          colour = FONT_MCOLOR_DKGRAY;
          break;
      }
      SetFontForeground(colour);

      const int16_t sNewX = sX + 1;
      const int16_t sNewY = sY + sHeight - 10;
      if (buffer == guiSAVEBUFFER) {
        RestoreExternBackgroundRect(sNewX, sNewY, 20, 15);
      }
      GPrintInvalidateF(sNewX, sNewY, L"%d", o.ubGunShotsLeft);

      // Display 'JAMMED' if we are jammed
      if (o.bGunAmmoStatus < 0) {
        SetFontForeground(FONT_MCOLOR_RED);

        const wchar_t *const jammed = sWidth >= BIG_INV_SLOT_WIDTH - 10
                                          ? TacticalStr[JAMMED_ITEM_STR]
                                          : TacticalStr[SHORT_JAMMED_GUN];

        int16_t cx;
        int16_t cy;
        FindFontCenterCoordinates(sX, sY, sWidth, sHeight, jammed, ITEM_FONT, &cx, &cy);
        GPrintInvalidate(cx, cy, jammed);
      }
    } else if (ubStatusIndex != RENDER_ITEM_NOSTATUS && o.ubNumberOfObjects > 1) {
      // Display # of items
      SetFontForeground(FONT_GRAY4);

      wchar_t pStr[16];
      swprintf(pStr, lengthof(pStr), L"%d", o.ubNumberOfObjects);

      const uint16_t uiStringLength = StringPixLength(pStr, ITEM_FONT);
      const int16_t sNewX = sX + sWidth - uiStringLength - 4;
      const int16_t sNewY = sY + sHeight - 10;

      if (buffer == guiSAVEBUFFER) {
        RestoreExternBackgroundRect(sNewX, sNewY, 15, 15);
      }
      GPrintInvalidate(sNewX, sNewY, pStr);
    }

    if (ItemHasAttachments(o)) {
      SetFontForeground(FindAttachment(&o, UNDER_GLAUNCHER) == NO_SLOT ? FONT_GREEN : FONT_YELLOW);

      const wchar_t *const attach_marker = L"*";
      uint16_t const uiStringLength = StringPixLength(attach_marker, ITEM_FONT);
      int16_t const sNewX = sX + sWidth - uiStringLength - 4;
      int16_t const sNewY = sY;

      if (buffer == guiSAVEBUFFER) {
        RestoreExternBackgroundRect(sNewX, sNewY, 15, 15);
      }
      GPrintInvalidate(sNewX, sNewY, attach_marker);
    }

    if (s && &o == &s->inv[HANDPOS] && Item[o.usItem].usItemClass == IC_GUN &&
        s->bWeaponMode != WM_NORMAL) {
      SetFontForeground(FONT_DKRED);

      const wchar_t *const mode_marker = s->bWeaponMode == WM_BURST ? L"*" : L"+";
      uint16_t const uiStringLength = StringPixLength(mode_marker, ITEM_FONT);
      int16_t const sNewX = sX + sWidth - uiStringLength - 4;
      int16_t const sNewY = sY + 13;  // rather arbitrary

      if (buffer == guiSAVEBUFFER) {
        RestoreExternBackgroundRect(sNewX, sNewY, 15, 15);
      }
      GPrintInvalidate(sNewX, sNewY, mode_marker);
    }
  }
}

BOOLEAN InItemDescriptionBox() { return (gfInItemDescBox); }

void CycleItemDescriptionItem() {
  int16_t usOldItem;

  // Delete old box...
  DeleteItemDescriptionBox();

  // Make new item....
  usOldItem = gpItemDescSoldier->inv[HANDPOS].usItem;

  if (IsKeyDown(SHIFT)) {
    usOldItem--;

    if (usOldItem < 0) {
      usOldItem = MAXITEMS - 1;
    }
  } else {
    usOldItem++;

    if (usOldItem > MAXITEMS) {
      usOldItem = 0;
    }
  }

  CreateItem((uint16_t)usOldItem, 100, &(gpItemDescSoldier->inv[HANDPOS]));

  InternalInitItemDescriptionBox(&(gpItemDescSoldier->inv[HANDPOS]), 214,
                                 (int16_t)(INV_INTERFACE_START_Y + 1), gubItemDescStatusIndex,
                                 gpItemDescSoldier);
}

void InitItemDescriptionBox(SOLDIERTYPE *pSoldier, uint8_t ubPosition, int16_t sX, int16_t sY,
                            uint8_t ubStatusIndex) {
  OBJECTTYPE *pObject;

  // DEF:
  // if we are in the shopkeeper screen, and we are to use the
  if (guiCurrentScreen == SHOPKEEPER_SCREEN && ubPosition == 255) {
    pObject = pShopKeeperItemDescObject;
  }

  // else use item from the hand position
  else {
    pObject = &(pSoldier->inv[ubPosition]);
  }

  InternalInitItemDescriptionBox(pObject, sX, sY, ubStatusIndex, pSoldier);
}

void InitKeyItemDescriptionBox(SOLDIERTYPE *const pSoldier, const uint8_t ubPosition,
                               const int16_t sX, const int16_t sY) {
  OBJECTTYPE *pObject;

  AllocateObject(&pObject);
  CreateKeyObject(pObject, pSoldier->pKeyRing[ubPosition].ubNumber,
                  pSoldier->pKeyRing[ubPosition].ubKeyID);

  InternalInitItemDescriptionBox(pObject, sX, sY, 0, pSoldier);
}

static void SetAttachmentTooltips() {
  for (uint32_t i = 0; i < MAX_ATTACHMENTS; ++i) {
    const uint16_t attachment = gpItemDescObject->usAttachItem[i];
    const wchar_t *const tip =
        (attachment != NOTHING ? ItemNames[attachment] : g_langRes->Message[STR_ATTACHMENTS]);
    gItemDescAttachmentRegions[i].SetFastHelpText(tip);
  }
}

static void BtnMoneyButtonCallback(GUI_BUTTON *btn, int32_t reason);
static void ItemDescAmmoCallback(GUI_BUTTON *btn, int32_t reason);
static void ItemDescAttachmentsCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void ItemDescCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void ItemDescDoneButtonCallback(GUI_BUTTON *btn, int32_t reason);
static void ReloadItemDesc();

void InternalInitItemDescriptionBox(OBJECTTYPE *const o, const int16_t sX, const int16_t sY,
                                    const uint8_t ubStatusIndex, SOLDIERTYPE *const s) {
  // Set the current screen
  guiCurrentItemDescriptionScreen = guiCurrentScreen;
  const BOOLEAN in_map = (guiCurrentItemDescriptionScreen == MAP_SCREEN);

  gsInvDescX = sX;
  gsInvDescY = sY;

  gpItemDescObject = o;
  gubItemDescStatusIndex = ubStatusIndex;
  gpItemDescSoldier = s;
  fItemDescDelete = FALSE;

  // Build a mouse region here that is over any others.....
  if (in_map) {
    MSYS_DefineRegion(&gInvDesc, gsInvDescX, gsInvDescY, gsInvDescX + MAP_ITEMDESC_WIDTH,
                      gsInvDescY + MAP_ITEMDESC_HEIGHT, MSYS_PRIORITY_HIGHEST - 2, CURSOR_NORMAL,
                      MSYS_NO_CALLBACK, ItemDescCallback);

    giMapInvDescButton =
        QuickCreateButtonImg(INTERFACEDIR "/itemdescdonebutton.sti", 0, 1, gsInvDescX + 204,
                             gsInvDescY + 107, MSYS_PRIORITY_HIGHEST, ItemDescDoneButtonCallback);

    fShowDescriptionFlag = TRUE;
  } else {
    MSYS_DefineRegion(&gInvDesc, gsInvDescX, gsInvDescY, gsInvDescX + ITEMDESC_WIDTH,
                      gsInvDescY + ITEMDESC_HEIGHT, MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR,
                      MSYS_NO_CALLBACK, ItemDescCallback);
  }

  if (Item[o->usItem].usItemClass & IC_GUN && o->usItem != ROCKET_LAUNCHER) {
    wchar_t pStr[10];
    swprintf(pStr, lengthof(pStr), L"%d/%d", o->ubGunShotsLeft, Weapon[o->usItem].ubMagSize);

    int32_t img;
    switch (o->ubGunAmmoType) {
      case AMMO_AP:
      case AMMO_SUPER_AP:
        img = 5;
        break;
      case AMMO_HP:
        img = 9;
        break;
      default:
        img = 1;
        break;
    }
    BUTTON_PICS *const ammo_img =
        LoadButtonImage(INTERFACEDIR "/infobox.sti", img + 3, img, -1, img + 2, -1);
    giItemDescAmmoButtonImages = ammo_img;

    const int16_t h = GetDimensionsOfButtonPic(ammo_img)->h;
    const SGPBox *const xy = (in_map ? &g_desc_item_box_map : &g_desc_item_box);
    const int16_t x = gsInvDescX + xy->x;
    const int16_t y = gsInvDescY + xy->y + xy->h - h;  // align with bottom
    const int16_t text_col = ITEMDESC_AMMO_FORE;
    const int16_t shadow_col = FONT_MCOLOR_BLACK;
    GUIButtonRef const ammo_btn =
        CreateIconAndTextButton(ammo_img, pStr, TINYFONT1, text_col, shadow_col, text_col,
                                shadow_col, x, y, MSYS_PRIORITY_HIGHEST, ItemDescAmmoCallback);
    giItemDescAmmoButton = ammo_btn;

    /* Disable the eject button, if we are being init from the shop keeper
     * screen and this is a dealer item we are getting info from */
    if (guiCurrentScreen == SHOPKEEPER_SCREEN && pShopKeeperItemDescObject) {
      ammo_btn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_HATCHED);
      DisableButton(ammo_btn);
    } else {
      ammo_btn->SetFastHelpText(g_langRes->Message[STR_EJECT_AMMO]);
    }

    int16_t usX;
    int16_t usY;
    FindFontCenterCoordinates(ITEMDESC_AMMO_TEXT_X, ITEMDESC_AMMO_TEXT_Y, ITEMDESC_AMMO_TEXT_WIDTH,
                              GetFontHeight(TINYFONT1), pStr, TINYFONT1, &usX, &usY);
    ammo_btn->SpecifyTextOffsets(usX, usY, TRUE);
  }

  if (ITEM_PROS_AND_CONS(o->usItem)) {
    int16_t const pros_cons_indent = std::max(StringPixLength(gzProsLabel, ITEMDESC_FONT),
                                              StringPixLength(gzConsLabel, ITEMDESC_FONT)) +
                                     10;
    const SGPBox *const box = (in_map ? &g_map_itemdesc_pros_cons_box : &g_itemdesc_pros_cons_box);
    uint16_t const x = box->x + pros_cons_indent + gsInvDescX;
    uint16_t y = box->y + gsInvDescY;
    uint16_t const w = box->w - pros_cons_indent;
    uint16_t const h = GetFontHeight(ITEMDESC_FONT);
    for (int32_t i = 0; i < 2; ++i) {
      // Add region for pros/cons help text
      MOUSE_REGION *const r = &gProsAndConsRegions[i];
      MSYS_DefineRegion(r, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR,
                        MSYS_NO_CALLBACK, ItemDescCallback);
      y += box->h;

      const wchar_t *label;
      // use temp variable to prevent an initial comma from being displayed
      wchar_t FullItemTemp[SIZE_ITEM_PROS];
      if (i == 0) {
        label = gzProsLabel;
        GenerateProsString(FullItemTemp, *o, 1000);
      } else {
        label = gzConsLabel;
        GenerateConsString(FullItemTemp, *o, 1000);
      }
      wchar_t text[SIZE_ITEM_PROS];
      swprintf(text, lengthof(text), L"%ls %ls", label, FullItemTemp);
      r->SetFastHelpText(text);
    }
  }

  // Load graphic
  guiItemDescBox = AddVideoObjectFromFile(INTERFACEDIR "/infobox.sti");
  guiMapItemDescBox = AddVideoObjectFromFile(INTERFACEDIR "/iteminfoc.sti");
  guiBullet = AddVideoObjectFromFile(INTERFACEDIR "/bullet.sti");

  if (o->usItem != MONEY) {
    const AttachmentGfxInfo *const agi = (in_map ? &g_map_attachment_info : &g_attachment_info);
    for (int32_t i = 0; i < MAX_ATTACHMENTS; ++i) {
      // Build a mouse region here that is over any others.....
      const uint16_t x = agi->item_box.x + agi->slot[i].iX + gsInvDescX;
      const uint16_t y = agi->item_box.y + agi->slot[i].iY + gsInvDescY;
      const uint16_t w = agi->item_box.w;
      const uint16_t h = agi->item_box.h;
      MOUSE_REGION *const r = &gItemDescAttachmentRegions[i];
      MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR,
                        MSYS_NO_CALLBACK, ItemDescAttachmentsCallback);
      MSYS_SetRegionUserData(r, 0, i);
    }
    SetAttachmentTooltips();
  } else {
    memset(&gRemoveMoney, 0, sizeof(REMOVE_MONEY));
    gRemoveMoney.uiTotalAmount = o->uiMoneyAmount;
    gRemoveMoney.uiMoneyRemaining = o->uiMoneyAmount;
    gRemoveMoney.uiMoneyRemoving = 0;

    guiMoneyGraphicsForDescBox = AddVideoObjectFromFile(INTERFACEDIR "/info_bil.sti");

    // Create buttons for the money
    guiMoneyButtonImage = LoadButtonImage(INTERFACEDIR "/info_bil.sti", 1, 2);
    const MoneyLoc *const loc = (in_map ? &gMapMoneyButtonLoc : &gMoneyButtonLoc);
    int32_t i;
    for (i = 0; i < MAX_ATTACHMENTS - 1; i++) {
      guiMoneyButtonBtn[i] = CreateIconAndTextButton(
          guiMoneyButtonImage, gzMoneyAmounts[i], BLOCKFONT2, 5, DEFAULT_SHADOW, 5, DEFAULT_SHADOW,
          loc->x + gMoneyButtonOffsets[i].x, loc->y + gMoneyButtonOffsets[i].y,
          MSYS_PRIORITY_HIGHEST, BtnMoneyButtonCallback);
      guiMoneyButtonBtn[i]->SetUserData(i);
    }
    if (gRemoveMoney.uiTotalAmount < 1000) DisableButton(guiMoneyButtonBtn[M_1000]);
    if (gRemoveMoney.uiTotalAmount < 100) DisableButton(guiMoneyButtonBtn[M_100]);
    if (gRemoveMoney.uiTotalAmount < 10) DisableButton(guiMoneyButtonBtn[M_10]);

    // Create the Done button
    guiMoneyDoneButtonImage = UseLoadedButtonImage(guiMoneyButtonImage, 3, 4);
    guiMoneyButtonBtn[i] = CreateIconAndTextButton(
        guiMoneyDoneButtonImage, gzMoneyAmounts[i], BLOCKFONT2, 5, DEFAULT_SHADOW, 5,
        DEFAULT_SHADOW, loc->x + gMoneyButtonOffsets[i].x, loc->y + gMoneyButtonOffsets[i].y,
        MSYS_PRIORITY_HIGHEST, BtnMoneyButtonCallback);
    guiMoneyButtonBtn[i]->SetUserData(i);
  }

  fInterfacePanelDirty = DIRTYLEVEL2;
  gfInItemDescBox = TRUE;

  ReloadItemDesc();

  gpAttachSoldier = (gpItemPointer ? gpItemPointerSoldier : s);
  // Store attachments that item originally had
  for (int32_t i = 0; i < MAX_ATTACHMENTS; ++i) {
    gusOriginalAttachItem[i] = o->usAttachItem[i];
    gbOriginalAttachStatus[i] = o->bAttachStatus[i];
  }

  if (gpItemPointer != NULL && !gfItemDescHelpTextOffset &&
      !CheckFact(FACT_ATTACHED_ITEM_BEFORE, 0)) {
    const wchar_t *text;
    if (!(Item[o->usItem].fFlags & ITEM_HIDDEN_ADDON) &&
        (ValidAttachment(gpItemPointer->usItem, o->usItem) ||
         ValidLaunchable(gpItemPointer->usItem, o->usItem) ||
         ValidMerge(gpItemPointer->usItem, o->usItem))) {
      text = g_langRes->Message[STR_ATTACHMENT_HELP];
    } else {
      text = g_langRes->Message[STR_ATTACHMENT_INVALID_HELP];
    }
    SetUpFastHelpRegion(69 + gsInvDescX, 12 + gsInvDescY, 170, text);

    StartShowingInterfaceFastHelpText();

    SetFactTrue(FACT_ATTACHED_ITEM_BEFORE);
    gfItemDescHelpTextOffset = TRUE;
  }
}

static void ReloadItemDesc() {
  guiItemGraphic = LoadTileGraphicForItem(Item[gpItemDescObject->usItem]);

  //
  // Load name, desc
  //

  // if the player is extracting money from the players account, use a different
  // item name and description
  uint16_t Item = gpItemDescObject->usItem;
  if (Item == MONEY && gfAddingMoneyToMercFromPlayersAccount) {
    Item = MONEY_FOR_PLAYERS_ACCOUNT;
  }
  gzItemName = ItemNames[Item];
  LoadItemInfo(Item, gzItemDesc);
}

static void ItemDescAmmoCallback(GUI_BUTTON *const btn, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (gpItemPointer) return;
    if (!EmptyWeaponMagazine(gpItemDescObject, &gItemPointer)) return;

    SetItemPointer(&gItemPointer, gpItemDescSoldier);
    fInterfacePanelDirty = DIRTYLEVEL2;

    btn->SpecifyText(L"0");

    if (guiCurrentItemDescriptionScreen == MAP_SCREEN) {
      SetMapCursorItem();
      fTeamPanelDirty = TRUE;
    } else {
      // if in SKI, load item into SKI's item pointer
      if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
        // pick up bullets from weapon into cursor (don't try to sell)
        BeginSkiItemPointer(PLAYERS_INVENTORY, -1, FALSE);
      }
      fItemDescDelete = TRUE;
    }
  }
}

static void DoAttachment() {
  if (AttachObject(gpItemDescSoldier, gpItemDescObject, gpItemPointer)) {
    if (gpItemPointer->usItem == NOTHING) {
      // attachment attached, merge item consumed, etc

      if (fInMapMode) {
        MAPEndItemPointer();
      } else {
        // End Item pickup
        gpItemPointer = NULL;
        EnableSMPanelButtons(TRUE, TRUE);

        gSMPanelRegion.ChangeCursor(CURSOR_NORMAL);
        SetCurrentCursorFromDatabase(CURSOR_NORMAL);

        // if we are currently in the shopkeeper interface
        if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
          // Clear out the moving cursor
          memset(&gMoveingItem, 0, sizeof(INVENTORY_IN_SLOT));

          // change the curosr back to the normal one
          SetSkiCursor(CURSOR_NORMAL);
        }
      }
    }

    if (gpItemDescObject->usItem == NOTHING) {
      // close desc panel panel
      DeleteItemDescriptionBox();
    } else {
      SetAttachmentTooltips();
    }
    // Dirty interface
    fInterfacePanelDirty = DIRTYLEVEL2;

    ReloadItemDesc();
  }

  // re-evaluate repairs
  gfReEvaluateEveryonesNothingToDo = TRUE;
}

static void PermanantAttachmentMessageBoxCallBack(MessageBoxReturnValue const ubExitValue) {
  if (ubExitValue == MSG_BOX_RETURN_YES) {
    DoAttachment();
  }
  // else do nothing
}

static void ItemDescAttachmentsCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  uint32_t uiItemPos;
  static BOOLEAN fRightDown = FALSE;

  if (gfItemDescObjectIsAttachment) {
    // screen out completely
    return;
  }

  uiItemPos = MSYS_GetRegionUserData(pRegion, 0);

  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // if the item being described belongs to a shopkeeper, ignore attempts to
    // pick it up / replace it
    if (guiCurrentScreen == SHOPKEEPER_SCREEN && pShopKeeperItemDescObject) {
      return;
    }

    // Try to place attachment if something is in our hand
    // require as many APs as to reload
    if (gpItemPointer != NULL) {
      // nb pointer could be NULL because of inventory manipulation in mapscreen
      // from sector inv
      if (!gpItemPointerSoldier || EnoughPoints(gpItemPointerSoldier, AP_RELOAD_GUN, 0, TRUE)) {
        if ((Item[gpItemPointer->usItem].fFlags & ITEM_INSEPARABLE) &&
            ValidAttachment(gpItemPointer->usItem, gpItemDescObject->usItem)) {
          DoScreenIndependantMessageBox(g_langRes->Message[STR_PERMANENT_ATTACHMENT],
                                        MSG_BOX_FLAG_YESNO, PermanantAttachmentMessageBoxCallBack);
          return;
        }

        DoAttachment();
      }
    } else {
      // ATE: Make sure we have enough AP's to drop it if we pick it up!
      if (EnoughPoints(gpItemDescSoldier, (AP_RELOAD_GUN + AP_PICKUP_ITEM), 0, TRUE)) {
        // Get attachment if there is one
        // The follwing function will handle if no attachment is here
        if (RemoveAttachment(gpItemDescObject, (uint8_t)uiItemPos, &gItemPointer)) {
          SetItemPointer(&gItemPointer, gpItemDescSoldier);

          //				if( guiCurrentScreen == MAP_SCREEN )
          if (guiCurrentItemDescriptionScreen == MAP_SCREEN) {
            SetMapCursorItem();
            fTeamPanelDirty = TRUE;
          }

          // if we are currently in the shopkeeper interface
          else if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
            // pick up attachment from item into cursor (don't try to sell)
            BeginSkiItemPointer(PLAYERS_INVENTORY, -1, FALSE);
          }

          // Dirty interface
          fInterfacePanelDirty = DIRTYLEVEL2;

          // re-evaluate repairs
          gfReEvaluateEveryonesNothingToDo = TRUE;

          UpdateItemHatches();
          SetAttachmentTooltips();
        }
      }
    }
  } else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    fRightDown = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP && fRightDown) {
    static OBJECTTYPE Object2;

    fRightDown = FALSE;

    if (gpItemDescObject->usAttachItem[uiItemPos] != NOTHING) {
      BOOLEAN fShopkeeperItem = FALSE;

      // remember if this is a shopkeeper's item we're viewing (
      // pShopKeeperItemDescObject will get nuked on deletion )
      if (guiCurrentScreen == SHOPKEEPER_SCREEN && pShopKeeperItemDescObject) {
        fShopkeeperItem = TRUE;
      }

      DeleteItemDescriptionBox();

      CreateItem(gpItemDescObject->usAttachItem[uiItemPos],
                 gpItemDescObject->bAttachStatus[uiItemPos], &Object2);

      gfItemDescObjectIsAttachment = TRUE;
      InternalInitItemDescriptionBox(&Object2, gsInvDescX, gsInvDescY, 0, gpItemDescSoldier);

      if (fShopkeeperItem) {
        pShopKeeperItemDescObject = &Object2;
        StartSKIDescriptionBox();
      }
    }
  }
}

static wchar_t const *GetObjectImprint(OBJECTTYPE const &o) {
  return !HasObjectImprint(o)              ? 0
         : o.ubImprintID == NO_PROFILE + 1 ? pwMiscSectorStrings[3]
                                           : GetProfile(o.ubImprintID).zNickname;
}

static void HighlightIf(const BOOLEAN cond) {
  SetFontForeground(cond ? ITEMDESC_FONTHIGHLIGHT : 5);
}

void RenderItemDescriptionBox() {
  if (!gfInItemDescBox) return;

  wchar_t pStr[100];
  int16_t usX;
  int16_t usY;

  OBJECTTYPE const &obj = *gpItemDescObject;
  BOOLEAN const in_map = guiCurrentItemDescriptionScreen == MAP_SCREEN;
  int16_t const dx = gsInvDescX;
  int16_t const dy = gsInvDescY;

  SGPVObject const *const box_gfx = in_map ? guiMapItemDescBox : guiItemDescBox;
  BltVideoObject(guiSAVEBUFFER, box_gfx, 0, dx, dy);

  // Display the money 'separating' border
  if (obj.usItem == MONEY) {  // Render the money Boxes
    MoneyLoc const &xy = in_map ? gMapMoneyButtonLoc : gMoneyButtonLoc;
    int32_t const x = xy.x + gMoneyButtonOffsets[0].x - 1;
    int32_t const y = xy.y + gMoneyButtonOffsets[0].y;
    BltVideoObject(guiSAVEBUFFER, guiMoneyGraphicsForDescBox, 0, x, y);
  }

  {  // Display item
    // center in slot, remove offsets
    ETRLEObject const &e = guiItemGraphic->SubregionProperties(0);
    SGPBox const &xy = in_map ? g_desc_item_box_map : g_desc_item_box;
    int32_t const x = dx + xy.x + (xy.w - e.usWidth) / 2 - e.sOffsetX;
    int32_t const y = dy + xy.y + (xy.h - e.usHeight) / 2 - e.sOffsetY;
    BltVideoObjectOutlineShadow(guiSAVEBUFFER, guiItemGraphic, 0, x - 2, y + 2);
    BltVideoObject(guiSAVEBUFFER, guiItemGraphic, 0, x, y);
  }

  {  // Display status
    SGPBox const &box = in_map ? g_map_itemdesc_item_status_box : g_itemdesc_item_status_box;
    int16_t const x = box.x + dx;
    int16_t const y = box.y + dy;
    int16_t const h = box.h;
    DrawItemUIBarEx(obj, gubItemDescStatusIndex, x, y, h, Get16BPPColor(DESC_STATUS_BAR),
                    Get16BPPColor(DESC_STATUS_BAR_SHADOW), guiSAVEBUFFER);
  }

  bool hatch_out_attachments = gfItemDescObjectIsAttachment;  // if examining attachment, always
                                                              // hatch out attachment slots
  if (OBJECTTYPE const *const ptr_obj = gpItemPointer) {
    if (Item[ptr_obj->usItem].fFlags & ITEM_HIDDEN_ADDON ||
        (!ValidItemAttachment(&obj, ptr_obj->usItem, FALSE) &&
         !ValidMerge(ptr_obj->usItem, obj.usItem) &&
         !ValidLaunchable(ptr_obj->usItem, obj.usItem))) {
      hatch_out_attachments = TRUE;
    }
  }

  {  // Display attachments
    AttachmentGfxInfo const &agi = in_map ? g_map_attachment_info : g_attachment_info;
    for (int32_t i = 0; i < MAX_ATTACHMENTS; ++i) {
      int16_t const x = dx + agi.slot[i].iX;
      int16_t const y = dy + agi.slot[i].iY;

      if (obj.usAttachItem[i] != NOTHING) {
        int16_t const item_x = agi.item_box.x + x;
        int16_t const item_y = agi.item_box.y + y;
        int16_t const item_w = agi.item_box.w;
        int16_t const item_h = agi.item_box.h;
        INVRenderItem(guiSAVEBUFFER, NULL, obj, item_x, item_y, item_w, item_h, DIRTYLEVEL2,
                      RENDER_ITEM_ATTACHMENT1 + i, SGP_TRANSPARENT);

        int16_t const bar_x = agi.bar_box.x + x;
        int16_t const bar_h = agi.bar_box.h;
        int16_t const bar_y = agi.bar_box.y + y + bar_h - 1;
        DrawItemUIBarEx(obj, DRAW_ITEM_STATUS_ATTACHMENT1 + i, bar_x, bar_y, bar_h,
                        Get16BPPColor(STATUS_BAR), Get16BPPColor(STATUS_BAR_SHADOW), guiSAVEBUFFER);
      }

      if (hatch_out_attachments) {
        uint16_t const hatch_w = agi.item_box.x + agi.item_box.w;
        uint16_t const hatch_h = agi.item_box.y + agi.item_box.h;
        DrawHatchOnInventory(guiSAVEBUFFER, x, y, hatch_w, hatch_h);
      }
    }
  }

  INVTYPE const &item = Item[obj.usItem];

  if (item.usItemClass & IC_GUN) {
    // display bullets for ROF
    {
      int32_t const x = in_map ? MAP_BULLET_SING_X : BULLET_SING_X;
      int32_t const y = in_map ? MAP_BULLET_SING_Y : BULLET_SING_Y;
      BltVideoObject(guiSAVEBUFFER, guiBullet, 0, x, y);
    }

    WEAPONTYPE const &w = Weapon[obj.usItem];
    if (w.ubShotsPerBurst > 0) {
      int32_t x = in_map ? MAP_BULLET_BURST_X : BULLET_BURST_X;
      int32_t const y = in_map ? MAP_BULLET_BURST_Y : BULLET_BURST_Y;
      for (int32_t i = w.ubShotsPerBurst; i != 0; --i) {
        BltVideoObject(guiSAVEBUFFER, guiBullet, 0, x, y);
        x += BULLET_WIDTH + 1;
      }
    }
  }

  {
    int16_t const w = in_map ? MAP_ITEMDESC_WIDTH : ITEMDESC_WIDTH;
    int16_t const h = in_map ? MAP_ITEMDESC_HEIGHT : ITEMDESC_HEIGHT;
    RestoreExternBackgroundRect(dx, dy, w, h);
  }

  // Render font desc
  SetFontAttributes(ITEMDESC_FONT, FONT_FCOLOR_WHITE);

  {  // Render name
    SGPBox const &xy = in_map ? gMapDescNameBox : gDescNameBox;
    MPrint(dx + xy.x, dy + xy.y, gzItemName);
  }

  SetFontShadow(ITEMDESC_FONTSHADOW2);

  {
    SGPBox const &box = in_map ? g_map_itemdesc_desc_box : g_itemdesc_desc_box;
    DisplayWrappedString(dx + box.x, dy + box.y, box.w, 2, ITEMDESC_FONT, FONT_BLACK, gzItemDesc,
                         FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
  }

  if (ITEM_PROS_AND_CONS(obj.usItem)) {
    {
      WEAPONTYPE const &w = Weapon[obj.usItem];
      size_t n = 0;
      if (w.ubCalibre != NOAMMO) {
        n += swprintf(pStr, lengthof(pStr), L"%ls ", AmmoCaliber[w.ubCalibre]);
      }
      n += swprintf(pStr + n, lengthof(pStr) - n, L"%ls", WeaponType[w.ubWeaponType]);
      if (wchar_t const *const imprint = GetObjectImprint(obj)) {  // Add name noting imprint
        n += swprintf(pStr + n, lengthof(pStr) - n, L" (%ls)", imprint);
      }

      SGPBox const &xy = in_map ? gMapDescNameBox : gDescNameBox;
      FindFontRightCoordinates(dx + xy.x, dy + xy.y, xy.w, xy.h, pStr, ITEMDESC_FONT, &usX, &usY);
      MPrint(usX, usY, pStr);
    }

    {
      SGPBox const &box = in_map ? g_map_itemdesc_pros_cons_box : g_itemdesc_pros_cons_box;
      int32_t x = box.x + dx;
      int32_t const y = box.y + dy;
      int32_t w = box.w;
      int32_t const h = box.h;

      SetFontForeground(FONT_MCOLOR_DKWHITE2);
      SetFontShadow(DEFAULT_SHADOW);
      MPrint(x, y, gzProsLabel);
      MPrint(x, y + h, gzConsLabel);

      SetFontForeground(FONT_BLACK);
      SetFontShadow(ITEMDESC_FONTSHADOW2);

      int16_t const pros_cons_indent = std::max(StringPixLength(gzProsLabel, ITEMDESC_FONT),
                                                StringPixLength(gzConsLabel, ITEMDESC_FONT)) +
                                       10;
      x += pros_cons_indent;
      w -= pros_cons_indent + StringPixLength(DOTDOTDOT, ITEMDESC_FONT);

      GenerateProsString(gzItemPros, obj, w);
      MPrint(x, y, gzItemPros);

      GenerateConsString(gzItemCons, obj, w);
      MPrint(x, y + h, gzItemCons);
    }
  }

  // Calculate total weight of item and attachments
  float fWeight = CalculateObjectWeight(&obj) / 10.f;
  if (!gGameSettings.fOptions[TOPTION_USE_METRIC_SYSTEM]) fWeight *= 2.2f;
  if (fWeight < 0.1) fWeight = 0.1f;

  SetFontShadow(DEFAULT_SHADOW);

  // Render, stat  name
  if (item.usItemClass & IC_WEAPON) {
    SetFontForeground(6);

    INV_DESC_STATS const *const ids = in_map ? gMapWeaponStats : gWeaponStats;

    // LABELS
    mprintf(dx + ids[0].sX, dy + ids[0].sY, gWeaponStatsDesc[0],
            GetWeightUnitString());  // mass
    if (item.usItemClass & (IC_GUN | IC_LAUNCHER)) {
      MPrint(dx + ids[2].sX, dy + ids[2].sY, gWeaponStatsDesc[3]);  // range
    }
    if (!(item.usItemClass & IC_LAUNCHER) && obj.usItem != ROCKET_LAUNCHER) {
      MPrint(dx + ids[3].sX, dy + ids[3].sY, gWeaponStatsDesc[4]);  // damage
    }
    MPrint(dx + ids[4].sX, dy + ids[4].sY, gWeaponStatsDesc[5]);  // APs
    if (item.usItemClass & IC_GUN) {
      MPrint(dx + ids[6].sX, dy + ids[6].sY, gWeaponStatsDesc[6]);  // = (sic)
    }
    MPrint(dx + ids[1].sX, dy + ids[1].sY, gWeaponStatsDesc[1]);  // status

    WEAPONTYPE const &w = Weapon[obj.usItem];
    if (w.ubShotsPerBurst > 0) {
      MPrint(dx + ids[7].sX, dy + ids[7].sY, gWeaponStatsDesc[6]);  // = (sic)
    }

    // Status
    SetFontForeground(5);
    swprintf(pStr, lengthof(pStr), L"%2d%%", obj.bGunStatus);
    FindFontRightCoordinates(dx + ids[1].sX + ids[1].sValDx, dy + ids[1].sY, ITEM_STATS_WIDTH,
                             ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
    MPrint(usX, usY, pStr);

    // Weight
    HighlightIf(fWeight <= EXCEPTIONAL_WEIGHT / 10);
    swprintf(pStr, lengthof(pStr), L"%1.1f", fWeight);
    FindFontRightCoordinates(dx + ids[0].sX + ids[0].sValDx, dy + ids[0].sY, ITEM_STATS_WIDTH,
                             ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
    MPrint(usX, usY, pStr);

    if (item.usItemClass & (IC_GUN | IC_LAUNCHER)) {  // Range
      uint16_t const range = GunRange(obj);
      HighlightIf(range >= EXCEPTIONAL_RANGE);
      swprintf(pStr, lengthof(pStr), L"%2d", range / 10);
      FindFontRightCoordinates(dx + ids[2].sX + ids[2].sValDx, dy + ids[2].sY, ITEM_STATS_WIDTH,
                               ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    }

    if (!(item.usItemClass & IC_LAUNCHER) && obj.usItem != ROCKET_LAUNCHER) {  // Damage
      HighlightIf(w.ubImpact >= EXCEPTIONAL_DAMAGE);
      swprintf(pStr, lengthof(pStr), L"%2d", w.ubImpact);
      FindFontRightCoordinates(dx + ids[3].sX + ids[3].sValDx, dy + ids[3].sY, ITEM_STATS_WIDTH,
                               ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    }

    uint8_t const ubAttackAPs = BaseAPsToShootOrStab(DEFAULT_APS, DEFAULT_AIMSKILL, obj);

    // APs
    HighlightIf(ubAttackAPs <= EXCEPTIONAL_AP_COST);
    swprintf(pStr, lengthof(pStr), L"%2d", ubAttackAPs);
    FindFontRightCoordinates(dx + ids[4].sX + ids[4].sValDx, dy + ids[4].sY, ITEM_STATS_WIDTH,
                             ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
    MPrint(usX, usY, pStr);

    if (w.ubShotsPerBurst > 0) {
      HighlightIf(w.ubShotsPerBurst >= EXCEPTIONAL_BURST_SIZE || obj.usItem == G11);
      swprintf(pStr, lengthof(pStr), L"%2d", ubAttackAPs + CalcAPsToBurst(DEFAULT_APS, obj));
      FindFontRightCoordinates(dx + ids[5].sX + ids[5].sValDx, dy + ids[5].sY, ITEM_STATS_WIDTH,
                               ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    }
  } else if (obj.usItem == MONEY) {
    SetFontForeground(FONT_WHITE);

    {  // Display the total amount of money
      SPrintMoney(pStr, in_map && gfAddingMoneyToMercFromPlayersAccount
                            ? LaptopSaveInfo.iCurrentBalance
                            : gRemoveMoney.uiTotalAmount);
      SGPBox const &xy = in_map ? gMapDescNameBox : gDescNameBox;
      FindFontRightCoordinates(dx + xy.x, dy + xy.y, xy.w, xy.h, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    }

    {  // Display the 'Separate' text
      SetFontForeground(in_map ? 5 : 6);
      MoneyLoc const &xy = in_map ? gMapMoneyButtonLoc : gMoneyButtonLoc;
      wchar_t const *const label =
          !in_map && gfAddingMoneyToMercFromPlayersAccount ? gzMoneyAmounts[5] : gzMoneyAmounts[4];
      MPrint(xy.x + gMoneyButtonOffsets[4].x, xy.y + gMoneyButtonOffsets[4].y, label);
    }

    SetFontForeground(6);

    INV_DESC_STATS const *const xy = in_map ? gMapMoneyStats : gMoneyStats;

    if (!in_map && gfAddingMoneyToMercFromPlayersAccount) {
      MPrint(dx + xy[0].sX, dy + xy[0].sY,
             gMoneyStatsDesc[MONEY_DESC_PLAYERS]);  // current ...
      MPrint(dx + xy[1].sX, dy + xy[1].sY,
             gMoneyStatsDesc[MONEY_DESC_BALANCE]);  // ... balance
      MPrint(dx + xy[2].sX, dy + xy[2].sY,
             gMoneyStatsDesc[MONEY_DESC_AMOUNT_2_WITHDRAW]);  // amount to ...
      MPrint(dx + xy[3].sX, dy + xy[3].sY,
             gMoneyStatsDesc[MONEY_DESC_TO_WITHDRAW]);  // ... widthdraw
    } else {
      MPrint(dx + xy[0].sX, dy + xy[0].sY,
             gMoneyStatsDesc[MONEY_DESC_AMOUNT]);  // amount ...
      MPrint(dx + xy[1].sX, dy + xy[1].sY,
             gMoneyStatsDesc[MONEY_DESC_REMAINING]);  // ... remaining
      MPrint(dx + xy[2].sX, dy + xy[2].sY,
             gMoneyStatsDesc[MONEY_DESC_AMOUNT_2_SPLIT]);  // amount ...
      MPrint(dx + xy[3].sX, dy + xy[3].sY,
             gMoneyStatsDesc[MONEY_DESC_TO_SPLIT]);  // ... to split
    }

    SetFontForeground(5);

    // Get length of string
    uint16_t const uiRightLength = 35;

    // Display the total amount of money remaining
    SPrintMoney(pStr, gRemoveMoney.uiMoneyRemaining);
    if (in_map) {
      uint16_t const uiStringLength = StringPixLength(pStr, ITEMDESC_FONT);
      int16_t const sStrX = dx + xy[1].sX + xy[1].sValDx + (uiRightLength - uiStringLength);
      MPrint(sStrX, dy + xy[1].sY, pStr);
    } else {
      FindFontRightCoordinates(dx + xy[1].sX + xy[1].sValDx, dy + xy[1].sY, ITEM_STATS_WIDTH - 3,
                               ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    }

    // Display the total amount of money removing
    SPrintMoney(pStr, gRemoveMoney.uiMoneyRemoving);
    if (in_map) {
      uint16_t const uiStringLength = StringPixLength(pStr, ITEMDESC_FONT);
      int16_t const sStrX = dx + xy[3].sX + xy[3].sValDx + (uiRightLength - uiStringLength);
      MPrint(sStrX, dy + xy[3].sY, pStr);
    } else {
      FindFontRightCoordinates(dx + xy[3].sX + xy[3].sValDx, dy + xy[3].sY, ITEM_STATS_WIDTH - 3,
                               ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    }
  } else if (item.usItemClass == IC_MONEY) {
    SetFontForeground(FONT_FCOLOR_WHITE);
    SPrintMoney(pStr, obj.uiMoneyAmount);
    SGPBox const &xy = in_map ? gMapDescNameBox : gDescNameBox;
    FindFontRightCoordinates(dx + xy.x, dy + xy.y, xy.w, xy.h, pStr, BLOCKFONT2, &usX, &usY);
    MPrint(usX, usY, pStr);
  } else {
    // Labels
    SetFontForeground(6);

    INV_DESC_STATS const *const ids = in_map ? gMapWeaponStats : gWeaponStats;

    /* amount for ammunition, status otherwise */
    wchar_t const *const label = Item[gpItemDescObject->usItem].usItemClass & IC_AMMO
                                     ? gWeaponStatsDesc[2]
                                     : gWeaponStatsDesc[1];
    MPrint(dx + ids[1].sX, dy + ids[1].sY, label);

    // Weight
    mprintf(dx + ids[0].sX, dy + ids[0].sY, gWeaponStatsDesc[0], GetWeightUnitString());

    // Values
    SetFontForeground(5);

    if (item.usItemClass & IC_AMMO) {  // Ammo - print amount
      swprintf(pStr, lengthof(pStr), L"%d/%d", obj.ubShotsLeft[0],
               Magazine[item.ubClassIndex].ubMagSize);
      FindFontRightCoordinates(dx + ids[1].sX + ids[1].sValDx, dy + ids[1].sY, ITEM_STATS_WIDTH,
                               ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    } else {  // Status
      swprintf(pStr, lengthof(pStr), L"%2d%%", obj.bStatus[gubItemDescStatusIndex]);
      FindFontRightCoordinates(dx + ids[1].sX + ids[1].sValDx, dy + ids[1].sY, ITEM_STATS_WIDTH,
                               ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    }

    // Weight
    swprintf(pStr, lengthof(pStr), L"%1.1f", fWeight);
    FindFontRightCoordinates(dx + ids[0].sX + ids[0].sValDx, dy + ids[0].sY, ITEM_STATS_WIDTH,
                             ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
    MPrint(usX, usY, pStr);

    if (InKeyRingPopup() || item.usItemClass & IC_KEY) {
      SetFontForeground(6);

      int32_t const x = dx + ids[3].sX;
      int32_t const y0 = dy + ids[3].sY;
      int32_t const y1 = y0 + GetFontHeight(BLOCKFONT) + 2;

      // build description for keys .. the sector found
      MPrint(x, y0, sKeyDescriptionStrings[0]);
      MPrint(x, y1, sKeyDescriptionStrings[1]);

      KEY const &key = KeyTable[obj.ubKeyID];

      SetFontForeground(5);
      wchar_t sTempString[128];
      GetShortSectorString(SECTORX(key.usSectorFound), SECTORY(key.usSectorFound), sTempString,
                           lengthof(sTempString));
      FindFontRightCoordinates(x, y0, 110, ITEM_STATS_HEIGHT, sTempString, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, sTempString);

      swprintf(pStr, lengthof(pStr), L"%d", key.usDateFound);
      FindFontRightCoordinates(x, y1, 110, ITEM_STATS_HEIGHT, pStr, BLOCKFONT2, &usX, &usY);
      MPrint(usX, usY, pStr);
    }
  }
}

void HandleItemDescriptionBox(DirtyLevel *const dirty_level) {
  if (fItemDescDelete) {
    DeleteItemDescriptionBox();
    fItemDescDelete = FALSE;
    *dirty_level = DIRTYLEVEL2;
  }
}

void DeleteItemDescriptionBox() {
  int32_t cnt, cnt2;
  BOOLEAN fFound, fAllFound;

  if (!gfInItemDescBox) return;

  //	DEF:

  // Used in the shopkeeper interface
  if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
    DeleteShopKeeperItemDescBox();
  }

  // check for any AP costs
  if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT)) {
    if (gpAttachSoldier) {
      // check for change in attachments, starting with removed attachments
      fAllFound = TRUE;
      for (cnt = 0; cnt < MAX_ATTACHMENTS; cnt++) {
        if (gusOriginalAttachItem[cnt] != NOTHING) {
          fFound = FALSE;
          for (cnt2 = 0; cnt2 < MAX_ATTACHMENTS; cnt2++) {
            if ((gusOriginalAttachItem[cnt] == gpItemDescObject->usAttachItem[cnt2]) &&
                (gpItemDescObject->bAttachStatus[cnt2] == gbOriginalAttachStatus[cnt])) {
              fFound = TRUE;
            }
          }
          if (!fFound) {
            // charge APs
            fAllFound = FALSE;
            break;
          }
        }
      }

      if (fAllFound) {
        // nothing was removed; search for attachment added
        for (cnt = 0; cnt < MAX_ATTACHMENTS; cnt++) {
          if (gpItemDescObject->usAttachItem[cnt] != NOTHING) {
            fFound = FALSE;
            for (cnt2 = 0; cnt2 < MAX_ATTACHMENTS; cnt2++) {
              if ((gpItemDescObject->usAttachItem[cnt] == gusOriginalAttachItem[cnt2]) &&
                  (gbOriginalAttachStatus[cnt2] == gpItemDescObject->bAttachStatus[cnt])) {
                fFound = TRUE;
              }
            }
            if (!fFound) {
              // charge APs
              fAllFound = FALSE;
              break;
            }
          }
        }
      }

      if (!fAllFound) {
        DeductPoints(gpAttachSoldier, AP_RELOAD_GUN, 0);
      }
    }
  }

  DeleteVideoObject(guiItemDescBox);
  DeleteVideoObject(guiMapItemDescBox);
  DeleteVideoObject(guiBullet);
  DeleteVideoObject(guiItemGraphic);

  gfInItemDescBox = FALSE;

  if (guiCurrentItemDescriptionScreen == MAP_SCREEN) {
    RemoveButton(giMapInvDescButton);
  }

  // Remove region
  MSYS_RemoveRegion(&gInvDesc);

  if (gpItemDescObject->usItem != MONEY) {
    for (cnt = 0; cnt < MAX_ATTACHMENTS; cnt++) {
      MSYS_RemoveRegion(&gItemDescAttachmentRegions[cnt]);
    }
  } else {
    UnloadButtonImage(guiMoneyButtonImage);
    UnloadButtonImage(guiMoneyDoneButtonImage);
    for (cnt = 0; cnt < MAX_ATTACHMENTS; cnt++) {
      RemoveButton(guiMoneyButtonBtn[cnt]);
    }
  }

  if (ITEM_PROS_AND_CONS(gpItemDescObject->usItem)) {
    MSYS_RemoveRegion(&gProsAndConsRegions[0]);
    MSYS_RemoveRegion(&gProsAndConsRegions[1]);
  }

  if (((Item[gpItemDescObject->usItem].usItemClass & IC_GUN) &&
       gpItemDescObject->usItem != ROCKET_LAUNCHER)) {
    // Remove button
    UnloadButtonImage(giItemDescAmmoButtonImages);
    RemoveButton(giItemDescAmmoButton);
  }
  if (guiCurrentItemDescriptionScreen == MAP_SCREEN) {
    fCharacterInfoPanelDirty = TRUE;
    fMapPanelDirty = TRUE;
    fTeamPanelDirty = TRUE;
    fMapScreenBottomDirty = TRUE;
  }

  if (InKeyRingPopup()) {
    DeleteKeyObject(gpItemDescObject);
    gpItemDescObject = NULL;
    fShowDescriptionFlag = FALSE;
    fInterfacePanelDirty = DIRTYLEVEL2;
    return;
  }

  fShowDescriptionFlag = FALSE;
  fInterfacePanelDirty = DIRTYLEVEL2;

  if (gpItemDescObject->usItem == MONEY) {
    // if there is no money remaining
    if (gRemoveMoney.uiMoneyRemaining == 0 && !gfAddingMoneyToMercFromPlayersAccount) {
      // get rid of the money in the slot
      memset(gpItemDescObject, 0, sizeof(OBJECTTYPE));
      gpItemDescObject = NULL;
    }
  }

  if (gfAddingMoneyToMercFromPlayersAccount) gfAddingMoneyToMercFromPlayersAccount = FALSE;

  gfItemDescObjectIsAttachment = FALSE;
}

void InternalBeginItemPointer(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObject, int8_t bHandPos) {
  //	BOOLEAN fOk;

  // If not null return
  if (gpItemPointer != NULL) {
    return;
  }

  // Copy into cursor...
  gItemPointer = *pObject;

  // Dirty interface
  fInterfacePanelDirty = DIRTYLEVEL2;
  SetItemPointer(&gItemPointer, pSoldier);
  gbItemPointerSrcSlot = bHandPos;
  gbItemPointerLocateGood = TRUE;

  CheckForDisabledForGiveItem();

  EnableSMPanelButtons(FALSE, TRUE);

  gfItemPointerDifferentThanDefault = FALSE;

  // re-evaluate repairs
  gfReEvaluateEveryonesNothingToDo = TRUE;
}

void BeginItemPointer(SOLDIERTYPE *pSoldier, uint8_t ubHandPos) {
  BOOLEAN fOk;
  OBJECTTYPE pObject;

  memset(&pObject, 0, sizeof(OBJECTTYPE));

  if (IsKeyDown(SHIFT)) {
    // Remove all from soldier's slot
    fOk = RemoveObjectFromSlot(pSoldier, ubHandPos, &pObject);
  } else {
    GetObjFrom(&(pSoldier->inv[ubHandPos]), 0, &pObject);
    fOk = (pObject.ubNumberOfObjects == 1);
  }
  if (fOk) {
    InternalBeginItemPointer(pSoldier, &pObject, ubHandPos);
  }
}

void BeginKeyRingItemPointer(SOLDIERTYPE *pSoldier, uint8_t ubKeyRingPosition) {
  BOOLEAN fOk;

  // If not null return
  if (gpItemPointer != NULL) {
    return;
  }

  if (IsKeyDown(SHIFT)) {
    // Remove all from soldier's slot
    fOk = RemoveKeysFromSlot(pSoldier, ubKeyRingPosition,
                             pSoldier->pKeyRing[ubKeyRingPosition].ubNumber, &gItemPointer);
  } else {
    RemoveKeyFromSlot(pSoldier, ubKeyRingPosition, &gItemPointer);
    fOk = (gItemPointer.ubNumberOfObjects == 1);
  }

  if (fOk) {
    // ATE: Look if we are a BLOODIED KNIFE, and change if so, making guy
    // scream...

    // Dirty interface
    fInterfacePanelDirty = DIRTYLEVEL2;
    SetItemPointer(&gItemPointer, pSoldier);
    gbItemPointerSrcSlot = ubKeyRingPosition;

    if (fInMapMode) SetMapCursorItem();
  } else {
    // Debug mesg
  }

  gfItemPointerDifferentThanDefault = FALSE;
}

void EndItemPointer() {
  if (gpItemPointer != NULL) {
    gpItemPointer = NULL;
    gbItemPointerSrcSlot = NO_SLOT;
    gSMPanelRegion.ChangeCursor(CURSOR_NORMAL);
    MSYS_SetCurrentCursor(CURSOR_NORMAL);

    if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
      memset(&gMoveingItem, 0, sizeof(INVENTORY_IN_SLOT));
      SetSkiCursor(CURSOR_NORMAL);
    } else {
      EnableSMPanelButtons(TRUE, TRUE);
    }

    gbItemPointerLocateGood = FALSE;

    // re-evaluate repairs
    gfReEvaluateEveryonesNothingToDo = TRUE;
  }
}

void DrawItemFreeCursor() {
  SetMouseCursorFromCurrentItem();
  gSMPanelRegion.ChangeCursor(EXTERN_CURSOR);
}

static BOOLEAN SoldierCanSeeCatchComing(const SOLDIERTYPE *pSoldier, int16_t sSrcGridNo) {
  return (TRUE);
  /*-
          int32_t							cnt;
          int8_t							bDirection,
  bTargetDirection;

          bTargetDirection = (int8_t)GetDirectionToGridNoFromGridNo(
  pSoldier->sGridNo, sSrcGridNo );

          // Look 3 directions Clockwise from what we are facing....
          bDirection = pSoldier->bDirection;

          for ( cnt = 0; cnt < 3; cnt++ )
          {
                  if ( bDirection == bTargetDirection )
                  {
                          return( TRUE );
                  }

                  bDirection = OneCDirection(bDirection);
          }

          // Look 3 directions CounterClockwise from what we are facing....
          bDirection = pSoldier->bDirection;

          for ( cnt = 0; cnt < 3; cnt++ )
          {
                  if ( bDirection == bTargetDirection )
                  {
                          return( TRUE );
                  }

                  bDirection = OneCCDirection(bDirection);
          }

          // If here, nothing good can happen!
          return( FALSE );
  -*/
}

void DrawItemTileCursor() {
  int16_t sAPCost;
  BOOLEAN fRecalc;
  int16_t sFinalGridNo;
  uint32_t uiCursorId = CURSOR_ITEM_GOOD_THROW;
  BOOLEAN fGiveItem = FALSE;
  int16_t sActionGridNo;
  static uint32_t uiOldCursorId = 0;
  static uint16_t usOldMousePos = 0;
  int16_t sEndZ = 0;
  int16_t sDist;
  int8_t bLevel;

  GridNo usMapPos = GetMouseMapPos();
  if (usMapPos != NOWHERE) {
    // Force mouse position to guy...
    if (gUIFullTarget != NULL) usMapPos = gUIFullTarget->sGridNo;

    gusCurMousePos = usMapPos;

    if (gusCurMousePos != usOldMousePos) {
      gfItemPointerDifferentThanDefault = FALSE;
    }

    // Save old one..
    usOldMousePos = gusCurMousePos;

    // Default to turning adjacent area gridno off....
    gfUIHandleShowMoveGrid = FALSE;

    // If we are over a talkable guy, set flag
    if (GetValidTalkableNPCFromMouse(TRUE, FALSE, TRUE) != NULL) {
      fGiveItem = TRUE;
    }

    // OK, if different than default, change....
    if (gfItemPointerDifferentThanDefault) {
      fGiveItem = !fGiveItem;
    }

    // Get recalc and cursor flags
    MouseMoveState uiCursorFlags;
    fRecalc = GetMouseRecalcAndShowAPFlags(&uiCursorFlags, NULL);

    // OK, if we begin to move, reset the cursor...
    if (uiCursorFlags != MOUSE_STATIONARY) {
      EndPhysicsTrajectoryUI();
    }

    // Get Pyth spaces away.....
    sDist = PythSpacesAway(gpItemPointerSoldier->sGridNo, gusCurMousePos);

    // If we are here and we are not selected, select!
    // ATE Design discussion propably needed here...
    SelectSoldier(gpItemPointerSoldier, SELSOLDIER_NONE);

    // ATE: if good for locate, locate to selected soldier....
    if (gbItemPointerLocateGood) {
      gbItemPointerLocateGood = FALSE;
      LocateSoldier(GetSelectedMan(), FALSE);
    }

    if (!fGiveItem) {
      if (UIHandleOnMerc(FALSE) && usMapPos != gpItemPointerSoldier->sGridNo) {
        // We are on a guy.. check if they can catch or not....
        const SOLDIERTYPE *const tgt = gUIFullTarget;
        if (tgt != NULL) {
          // Are they on our team?
          // ATE: Can't be an EPC
          if (tgt->bTeam == OUR_TEAM && !AM_AN_EPC(tgt) &&
              !(tgt->uiStatusFlags & SOLDIER_VEHICLE)) {
            if (sDist <= PASSING_ITEM_DISTANCE_OKLIFE) {
              // OK, on a valid pass
              gfUIMouseOnValidCatcher = 4;
              gUIValidCatcher = tgt;
            } else {
              // Can they see the throw?
              if (SoldierCanSeeCatchComing(tgt, gpItemPointerSoldier->sGridNo)) {
                // OK, set global that this buddy can see catch...
                gfUIMouseOnValidCatcher = TRUE;
                gUIValidCatcher = tgt;
              }
            }
          }
        }
      }

      // We're going to toss it!
      if (gTacticalStatus.uiFlags & INCOMBAT) {
        gfUIDisplayActionPoints = TRUE;
        gUIDisplayActionPointsOffX = 15;
        gUIDisplayActionPointsOffY = 15;
      }

      // If we are tossing...
      if ((sDist <= 1 && gfUIMouseOnValidCatcher == 0) || gfUIMouseOnValidCatcher == 4) {
        gsCurrentActionPoints = AP_PICKUP_ITEM;
      } else {
        gsCurrentActionPoints = AP_TOSS_ITEM;
      }

    } else {
      const SOLDIERTYPE *const tgt = gUIFullTarget;
      if (tgt != NULL) {
        UIHandleOnMerc(FALSE);

        // OK, set global that this buddy can see catch...
        gfUIMouseOnValidCatcher = 2;
        gUIValidCatcher = tgt;

        // If this is a robot, change to say 'reload'
        if (tgt->uiStatusFlags & SOLDIER_ROBOT) {
          gfUIMouseOnValidCatcher = 3;
        }

        if (uiCursorFlags == MOUSE_STATIONARY) {
          // Find adjacent gridno...
          sActionGridNo =
              FindAdjacentGridEx(gpItemPointerSoldier, gusCurMousePos, NULL, NULL, FALSE, FALSE);
          if (sActionGridNo == -1) {
            sActionGridNo = gusCurMousePos;
          }

          // Display location...
          gsUIHandleShowMoveGridLocation = sActionGridNo;
          gfUIHandleShowMoveGrid = TRUE;

          // Get AP cost
          if (tgt->uiStatusFlags & SOLDIER_ROBOT) {
            sAPCost = GetAPsToReloadRobot(gpItemPointerSoldier, tgt);
          } else {
            sAPCost = GetAPsToGiveItem(gpItemPointerSoldier, sActionGridNo);
          }

          gsCurrentActionPoints = sAPCost;
        }

        // Set value
        if (gTacticalStatus.uiFlags & INCOMBAT) {
          gfUIDisplayActionPoints = TRUE;
          gUIDisplayActionPointsOffX = 15;
          gUIDisplayActionPointsOffY = 15;
        }
      }
    }

    if (fGiveItem) {
      uiCursorId = CURSOR_ITEM_GIVE;
    } else {
      // How afar away are we?
      if (sDist <= 1 && gfUIMouseOnValidCatcher == 0) {
        // OK, we want to drop.....

        // Write the word 'drop' on cursor...
        SetIntTileLocationText(pMessageStrings[MSG_DROP]);
      } else {
        if (usMapPos == gpItemPointerSoldier->sGridNo) {
          EndPhysicsTrajectoryUI();
        } else if (gfUIMouseOnValidCatcher == 4) {
          // ATE: Don't do if we are passing....
        } else
        // ( sDist > PASSING_ITEM_DISTANCE_OKLIFE )
        {
          // Write the word 'drop' on cursor...
          if (gfUIMouseOnValidCatcher == 0) {
            SetIntTileLocationText(pMessageStrings[MSG_THROW]);
          }

          gfUIHandlePhysicsTrajectory = TRUE;

          if (fRecalc && usMapPos != gpItemPointerSoldier->sGridNo) {
            if (gfUIMouseOnValidCatcher) {
              switch (gAnimControl[gUIValidCatcher->usAnimState].ubHeight) {
                case ANIM_STAND:

                  sEndZ = 150;
                  break;

                case ANIM_CROUCH:

                  sEndZ = 80;
                  break;

                case ANIM_PRONE:

                  sEndZ = 10;
                  break;
              }

              if (gUIValidCatcher->bLevel > 0) sEndZ = 0;
            }

            // Calculate chance to throw here.....
            if (!CalculateLaunchItemChanceToGetThrough(gpItemPointerSoldier, gpItemPointer,
                                                       usMapPos, (int8_t)gsInterfaceLevel,
                                                       (int16_t)((gsInterfaceLevel * 256) + sEndZ),
                                                       &sFinalGridNo, FALSE, &bLevel, TRUE)) {
              gfBadThrowItemCTGH = TRUE;
            } else {
              gfBadThrowItemCTGH = FALSE;
            }

            BeginPhysicsTrajectoryUI(sFinalGridNo, bLevel, gfBadThrowItemCTGH);
          }
        }

        if (gfBadThrowItemCTGH) {
          uiCursorId = CURSOR_ITEM_BAD_THROW;
        }
      }
    }

    // Erase any cursor in viewport
    // gViewportRegion.ChangeCursor(VIDEO_NO_CURSOR);

    // Get tile graphic fro item
    uint16_t const usIndex = GetTileGraphicForItem(Item[gpItemPointer->usItem]);

    // ONly load if different....
    if (usIndex != gusItemPointer || uiOldCursorId != uiCursorId) {
      // OK, Tile database gives me subregion and video object to use...
      const TILE_ELEMENT *const te = &gTileDatabase[usIndex];
      SetExternVOData(uiCursorId, te->hTileSurface, te->usRegionIndex);
      gusItemPointer = usIndex;
      uiOldCursorId = uiCursorId;
    }

    gViewportRegion.ChangeCursor(uiCursorId);
  }
}

static bool IsValidAmmoToReloadRobot(SOLDIERTYPE const &s, OBJECTTYPE const &ammo) {
  OBJECTTYPE const &weapon = s.inv[HANDPOS];
  if (!CompatibleAmmoForGun(&ammo, &weapon)) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[ROBOT_NEEDS_GIVEN_CALIBER_STR],
              AmmoCaliber[Weapon[weapon.usItem].ubCalibre]);
    return false;
  }
  return true;
}

BOOLEAN HandleItemPointerClick(uint16_t usMapPos) {
  // Determine what to do
  uint8_t ubDirection;
  uint16_t usItem;
  int16_t sAPCost;
  uint8_t ubThrowActionCode = 0;
  int16_t sEndZ = 0;
  OBJECTTYPE TempObject;
  int16_t sGridNo;
  int16_t sDist;
  int16_t sDistVisible;

  if (SelectedGuyInBusyAnimation()) {
    return (FALSE);
  }

  if (g_ui_message_overlay != NULL) {
    EndUIMessage();
    return (FALSE);
  }

  // Don't allow if our soldier is a # of things...
  if (AM_AN_EPC(gpItemPointerSoldier) || gpItemPointerSoldier->bLife < OKLIFE ||
      gpItemPointerSoldier->bOverTerrainType == DEEP_WATER) {
    return (FALSE);
  }

  // This implies we have no path....
  if (gsCurrentActionPoints == 0) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NO_PATH]);
    return (FALSE);
  }

  if (gUIFullTarget != NULL) {
    // Force mouse position to guy...
    usMapPos = gUIFullTarget->sGridNo;

    if (gAnimControl[gUIFullTarget->usAnimState].uiFlags & ANIM_MOVING) {
      return (FALSE);
    }
  }

  // Check if we have APs....
  if (!EnoughPoints(gpItemPointerSoldier, gsCurrentActionPoints, 0, TRUE)) {
    if (gfDontChargeAPsToPickup && gsCurrentActionPoints == AP_PICKUP_ITEM) {
    } else {
      return (FALSE);
    }
  }

  // SEE IF WE ARE OVER A TALKABLE GUY!
  SOLDIERTYPE *const tgt = gUIFullTarget;
  BOOLEAN fGiveItem = tgt != NULL && IsValidTalkableNPC(tgt, TRUE, FALSE, TRUE);

  // OK, if different than default, change....
  if (gfItemPointerDifferentThanDefault) {
    fGiveItem = !fGiveItem;
  }

  // Get Pyth spaces away.....
  sDist = PythSpacesAway(gpItemPointerSoldier->sGridNo, gusCurMousePos);

  if (fGiveItem) {
    usItem = gpItemPointer->usItem;

    // If the target is a robot,
    if (tgt->uiStatusFlags & SOLDIER_ROBOT) {
      // Charge APs to reload robot!
      sAPCost = GetAPsToReloadRobot(gpItemPointerSoldier, tgt);
    } else {
      // Calculate action point costs!
      sAPCost = GetAPsToGiveItem(gpItemPointerSoldier, usMapPos);
    }

    // Place it back in our hands!

    TempObject = *gpItemPointer;

    if (gbItemPointerSrcSlot != NO_SLOT) {
      PlaceObject(gpItemPointerSoldier, gbItemPointerSrcSlot, gpItemPointer);
      fInterfacePanelDirty = DIRTYLEVEL2;
    }
    /*
                    //if the user just clicked on an arms dealer
                    if (IsMercADealer(tgt->ubProfile))
                    {
                            if ( EnoughPoints( gpItemPointerSoldier, sAPCost, 0,
       TRUE ) )
                            {
                                    //Enter the shopkeeper interface
                                    EnterShopKeeperInterfaceScreen(tgt->ubProfile);

                                    EndItemPointer( );
                            }

                            return( TRUE );
                    }
    */

    if (EnoughPoints(gpItemPointerSoldier, sAPCost, 0, TRUE)) {
      // If we are a robot, check if this is proper item to reload!
      if (tgt->uiStatusFlags & SOLDIER_ROBOT) {
        // Check if we can reload robot....
        if (IsValidAmmoToReloadRobot(*tgt, TempObject)) {
          int16_t sActionGridNo;
          uint8_t ubDirection;
          int16_t sAdjustedGridNo;

          // Walk up to him and reload!
          // See if we can get there to stab
          sActionGridNo = FindAdjacentGridEx(gpItemPointerSoldier, tgt->sGridNo, &ubDirection,
                                             &sAdjustedGridNo, TRUE, FALSE);

          if (sActionGridNo != -1 && gbItemPointerSrcSlot != NO_SLOT) {
            // Make a temp object for ammo...
            gpItemPointerSoldier->pTempObject = MALLOC(OBJECTTYPE);
            *gpItemPointerSoldier->pTempObject = TempObject;

            // Remove from soldier's inv...
            RemoveObjs(&(gpItemPointerSoldier->inv[gbItemPointerSrcSlot]), 1);

            gpItemPointerSoldier->sPendingActionData2 = sAdjustedGridNo;
            gpItemPointerSoldier->uiPendingActionData1 = gbItemPointerSrcSlot;
            gpItemPointerSoldier->bPendingActionData3 = ubDirection;
            gpItemPointerSoldier->ubPendingActionAnimCount = 0;

            // CHECK IF WE ARE AT THIS GRIDNO NOW
            if (gpItemPointerSoldier->sGridNo != sActionGridNo) {
              // SEND PENDING ACTION
              gpItemPointerSoldier->ubPendingAction = MERC_RELOADROBOT;

              // WALK UP TO DEST FIRST
              EVENT_InternalGetNewSoldierPath(gpItemPointerSoldier, sActionGridNo,
                                              gpItemPointerSoldier->usUIMovementMode, FALSE, FALSE);
            } else {
              EVENT_SoldierBeginReloadRobot(gpItemPointerSoldier, sAdjustedGridNo, ubDirection,
                                            gbItemPointerSrcSlot);
            }

            // OK, set UI
            SetUIBusy(gpItemPointerSoldier);
          }
        }

        gfDontChargeAPsToPickup = FALSE;
        EndItemPointer();
      } else {
        // if (gbItemPointerSrcSlot != NO_SLOT )
        {
          // Give guy this item.....
          SoldierGiveItem(gpItemPointerSoldier, tgt, &TempObject, gbItemPointerSrcSlot);

          gfDontChargeAPsToPickup = FALSE;
          EndItemPointer();

          // If we are giving it to somebody not on our team....
          if (tgt->ubProfile < FIRST_RPC || RPC_RECRUITED(tgt)) {
          } else {
            SetEngagedInConvFromPCAction(gpItemPointerSoldier);
          }
        }
      }
    }

    return (TRUE);
  }

  // CHECK IF WE ARE NOT ON THE SAME GRIDNO
  if (sDist <= 1 && (gUIFullTarget == NULL || gUIFullTarget == gpItemPointerSoldier)) {
    // Check some things here....
    // 1 ) are we at the exact gridno that we stand on?
    if (usMapPos == gpItemPointerSoldier->sGridNo) {
      // Drop
      if (!gfDontChargeAPsToPickup) {
        // Deduct points
        DeductPoints(gpItemPointerSoldier, AP_PICKUP_ITEM, 0);
      }

      SoldierDropItem(gpItemPointerSoldier, gpItemPointer);
    } else {
      // Try to drop in an adjacent area....
      // 1 ) is this not a good OK destination
      // this will sound strange, but this is OK......
      if (!NewOKDestination(gpItemPointerSoldier, usMapPos, FALSE, gpItemPointerSoldier->bLevel) ||
          FindBestPath(gpItemPointerSoldier, usMapPos, gpItemPointerSoldier->bLevel, WALKING,
                       NO_COPYROUTE, 0) == 1) {
        // Drop
        if (!gfDontChargeAPsToPickup) {
          // Deduct points
          DeductPoints(gpItemPointerSoldier, AP_PICKUP_ITEM, 0);
        }

        // Play animation....
        // Don't show animation of dropping item, if we are not standing

        switch (gAnimControl[gpItemPointerSoldier->usAnimState].ubHeight) {
          case ANIM_STAND:
            gpItemPointerSoldier->pTempObject = MALLOC(OBJECTTYPE);
            *gpItemPointerSoldier->pTempObject = *gpItemPointer;
            gpItemPointerSoldier->sPendingActionData2 = usMapPos;

            // Turn towards.....gridno
            EVENT_SetSoldierDesiredDirectionForward(
                gpItemPointerSoldier,
                (int8_t)GetDirectionFromGridNo(usMapPos, gpItemPointerSoldier));

            EVENT_InitNewSoldierAnim(gpItemPointerSoldier, DROP_ADJACENT_OBJECT, 0, FALSE);
            break;

          case ANIM_CROUCH:
          case ANIM_PRONE:
            AddItemToPool(usMapPos, gpItemPointer, VISIBLE, gpItemPointerSoldier->bLevel, 0, -1);
            NotifySoldiersToLookforItems();
            break;
        }
      } else {
        // Drop in place...
        if (!gfDontChargeAPsToPickup) {
          // Deduct points
          DeductPoints(gpItemPointerSoldier, AP_PICKUP_ITEM, 0);
        }

        SoldierDropItem(gpItemPointerSoldier, gpItemPointer);
      }
    }
  } else {
    sGridNo = usMapPos;

    SOLDIERTYPE *const pSoldier = gUIFullTarget;
    if (sDist <= PASSING_ITEM_DISTANCE_OKLIFE && pSoldier != NULL && pSoldier->bTeam == OUR_TEAM &&
        !AM_AN_EPC(pSoldier) && !(pSoldier->uiStatusFlags & SOLDIER_VEHICLE)) {
      // OK, do the transfer...
      {
        {
          if (!EnoughPoints(pSoldier, 3, 0, TRUE) ||
              !EnoughPoints(gpItemPointerSoldier, 3, 0, TRUE)) {
            return (FALSE);
          }

          sDistVisible =
              DistanceVisible(pSoldier, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT,
                              gpItemPointerSoldier->sGridNo, gpItemPointerSoldier->bLevel);

          // Check LOS....
          if (!SoldierTo3DLocationLineOfSightTest(pSoldier, gpItemPointerSoldier->sGridNo,
                                                  gpItemPointerSoldier->bLevel, 3,
                                                  (uint8_t)sDistVisible, TRUE)) {
            return (FALSE);
          }

          // Charge AP values...
          DeductPoints(pSoldier, 3, 0);
          DeductPoints(gpItemPointerSoldier, 3, 0);

          usItem = gpItemPointer->usItem;

          // try to auto place object....
          if (AutoPlaceObject(pSoldier, gpItemPointer, TRUE)) {
            ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_ITEM_PASSED_TO_MERC],
                      ShortItemNames[usItem], pSoldier->name);

            // Check if it's the same now!
            if (gpItemPointer->ubNumberOfObjects == 0) {
              EndItemPointer();
            }

            // OK, make guys turn towards each other and do animation...
            {
              uint8_t ubFacingDirection;

              // Get direction to face.....
              ubFacingDirection =
                  (uint8_t)GetDirectionFromGridNo(gpItemPointerSoldier->sGridNo, pSoldier);

              // Stop merc first....
              EVENT_StopMerc(pSoldier);

              // If we are standing only...
              if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_STAND &&
                  !MercInWater(pSoldier)) {
                // Turn to face, then do animation....
                EVENT_SetSoldierDesiredDirection(pSoldier, ubFacingDirection);
                pSoldier->fTurningUntilDone = TRUE;
                pSoldier->usPendingAnimation = PASS_OBJECT;
              }

              if (gAnimControl[gpItemPointerSoldier->usAnimState].ubEndHeight == ANIM_STAND &&
                  !MercInWater(gpItemPointerSoldier)) {
                EVENT_SetSoldierDesiredDirection(gpItemPointerSoldier,
                                                 OppositeDirection(ubFacingDirection));
                gpItemPointerSoldier->fTurningUntilDone = TRUE;
                gpItemPointerSoldier->usPendingAnimation = PASS_OBJECT;
              }
            }

            return (TRUE);
          } else {
            ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                      pMessageStrings[MSG_NO_ROOM_TO_PASS_ITEM], ShortItemNames[usItem],
                      pSoldier->name);
            return (FALSE);
          }
        }
      }
    } else {
      // CHECK FOR VALID CTGH
      if (gfBadThrowItemCTGH) {
        ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[CANNOT_THROW_TO_DEST_STR]);
        return (FALSE);
      }

      // Deduct points
      // DeductPoints( gpItemPointerSoldier, AP_TOSS_ITEM, 0 );
      gpItemPointerSoldier->fDontChargeTurningAPs = TRUE;
      // Will be dome later....

      ubThrowActionCode = NO_THROW_ACTION;

      // OK, CHECK FOR VALID THROW/CATCH
      // IF OVER OUR GUY...
      SOLDIERTYPE *target = NULL;
      if (pSoldier != NULL) {
        if (pSoldier->bTeam == OUR_TEAM && pSoldier->bLife >= OKLIFE && !AM_AN_EPC(pSoldier) &&
            !(pSoldier->uiStatusFlags & SOLDIER_VEHICLE)) {
          // OK, on our team,

          // How's our direction?
          if (SoldierCanSeeCatchComing(pSoldier, gpItemPointerSoldier->sGridNo)) {
            // Setup as being the catch target
            ubThrowActionCode = THROW_TARGET_MERC_CATCH;
            target = pSoldier;

            sGridNo = pSoldier->sGridNo;

            switch (gAnimControl[pSoldier->usAnimState].ubHeight) {
              case ANIM_STAND:

                sEndZ = 150;
                break;

              case ANIM_CROUCH:

                sEndZ = 80;
                break;

              case ANIM_PRONE:

                sEndZ = 10;
                break;
            }

            if (pSoldier->bLevel > 0) {
              sEndZ = 0;
            }

            // Get direction
            ubDirection = (uint8_t)GetDirectionFromGridNo(gpItemPointerSoldier->sGridNo, pSoldier);

            // ATE: Goto stationary...
            SoldierGotoStationaryStance(pSoldier);

            // Set direction to turn...
            EVENT_SetSoldierDesiredDirection(pSoldier, ubDirection);
          }
        }
      }

      // CHANGE DIRECTION AT LEAST
      ubDirection = (uint8_t)GetDirectionFromGridNo(sGridNo, gpItemPointerSoldier);
      EVENT_SetSoldierDesiredDirection(gpItemPointerSoldier, ubDirection);
      gpItemPointerSoldier->fTurningUntilDone = TRUE;

      // Increment attacker count...
      gTacticalStatus.ubAttackBusyCount++;
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("INcremtning ABC: Throw item to %d", gTacticalStatus.ubAttackBusyCount));

      // Given our gridno, throw grenate!
      CalculateLaunchItemParamsForThrow(gpItemPointerSoldier, sGridNo, gpItemPointerSoldier->bLevel,
                                        gsInterfaceLevel * 256 + sEndZ, gpItemPointer, 0,
                                        ubThrowActionCode, target);

      // OK, goto throw animation
      HandleSoldierThrowItem(gpItemPointerSoldier, usMapPos);
    }
  }

  gfDontChargeAPsToPickup = FALSE;
  EndItemPointer();

  return (TRUE);
}

BOOLEAN InItemStackPopup() { return (gfInItemStackPopup); }

BOOLEAN InKeyRingPopup() { return (gfInKeyRingPopup); }

static void ItemPopupFullRegionCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void ItemPopupRegionCallback(MOUSE_REGION *pRegion, int32_t iReason);

void InitItemStackPopup(SOLDIERTYPE *const pSoldier, uint8_t const ubPosition, int16_t const sInvX,
                        int16_t const sInvY, int16_t const sInvWidth, int16_t const sInvHeight) {
  SGPRect aRect;
  uint8_t ubLimit;
  uint8_t ubCols;
  uint8_t ubRows;
  int32_t cnt;

  // Set some globals
  gsItemPopupInvX = sInvX;
  gsItemPopupInvY = sInvY;
  gsItemPopupInvWidth = sInvWidth;
  gsItemPopupInvHeight = sInvHeight;

  gpItemPopupSoldier = pSoldier;

  // Determine # of items
  gpItemPopupObject = &(pSoldier->inv[ubPosition]);
  ubLimit = ItemSlotLimit(gpItemPopupObject->usItem, ubPosition);

  // Return if #objects not >1
  if (ubLimit < 1) return;

  if (ubLimit > MAX_STACK_POPUP_WIDTH) {
    ubCols = MAX_STACK_POPUP_WIDTH;
    ubRows = ubLimit / MAX_STACK_POPUP_WIDTH;
  } else {
    ubCols = ubLimit;
    ubRows = 0;
  }

  // Load graphics
  guiItemPopupBoxes = AddVideoObjectFromFile(INTERFACEDIR "/extra_inventory.sti");

  // Get size
  ETRLEObject const &pTrav = guiItemPopupBoxes->SubregionProperties(0);
  uint16_t const usPopupWidth = pTrav.usWidth;
  uint16_t const usPopupHeight = pTrav.usHeight;

  // Get Width, Height
  int16_t gsItemPopupWidth = ubCols * usPopupWidth;
  int16_t gsItemPopupHeight = ubRows * usPopupHeight;
  gubNumItemPopups = ubLimit;

  // Calculate X,Y, first center
  MOUSE_REGION const &r = gSMInvRegion[ubPosition];
  int16_t sCenX = r.X() - (gsItemPopupWidth / 2 + r.W() / 2);
  int16_t sCenY = r.Y() - (gsItemPopupHeight / 2 + r.H() / 2);

  // Limit it to window for item desc
  if (sCenX < gsItemPopupInvX) {
    sCenX = gsItemPopupInvX;
  }
  if ((sCenX + gsItemPopupWidth) > (gsItemPopupInvX + gsItemPopupInvWidth)) {
    sCenX = gsItemPopupInvX + gsItemPopupInvWidth - gsItemPopupWidth;
  }
  if (sCenY < gsItemPopupInvY) {
    sCenY = gsItemPopupInvY;
  }
  if (sCenY + gsItemPopupHeight > (gsItemPopupInvY + gsItemPopupInvHeight)) {
    sCenY = gsItemPopupInvY + gsItemPopupInvHeight - gsItemPopupHeight;
  }

  // Cap it at 0....
  if (sCenX < 0) {
    sCenX = 0;
  }
  if (sCenY < 0) {
    sCenY = 0;
  }

  // Set
  gsItemPopupX = sCenX;
  gsItemPopupY = sCenY;

  for (cnt = 0; cnt < gubNumItemPopups; cnt++) {
    uint32_t row = cnt / MAX_STACK_POPUP_WIDTH;
    uint32_t col = cnt % MAX_STACK_POPUP_WIDTH;

    // Build a mouse region here that is over any others.....
    MSYS_DefineRegion(&gItemPopupRegions[cnt], sCenX + col * usPopupWidth,
                      sCenY + row * usPopupHeight, sCenX + (col + 1) * usPopupWidth,
                      sCenY + (row + 1) * usPopupHeight, MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR,
                      MSYS_NO_CALLBACK, ItemPopupRegionCallback);
    MSYS_SetRegionUserData(&gItemPopupRegions[cnt], 0, cnt);

    // OK, for each item, set dirty text if applicable!
    gItemPopupRegions[cnt].SetFastHelpText(ItemNames[pSoldier->inv[ubPosition].usItem]);
    gfItemPopupRegionCallbackEndFix = FALSE;
  }

  // Build a mouse region here that is over any others.....
  MSYS_DefineRegion(&gItemPopupRegion, gsItemPopupInvX, gsItemPopupInvY,
                    gsItemPopupInvX + gsItemPopupInvWidth, gsItemPopupInvY + gsItemPopupInvHeight,
                    MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK,
                    ItemPopupFullRegionCallback);

  // Disable all faces
  SetAllAutoFacesInactive();

  fInterfacePanelDirty = DIRTYLEVEL2;

  gfInItemStackPopup = TRUE;

  if (guiCurrentItemDescriptionScreen != MAP_SCREEN) {
    EnableSMPanelButtons(FALSE, FALSE);
  }

  // Reserict mouse cursor to panel
  aRect.iTop = sInvY;
  aRect.iLeft = sInvX;
  aRect.iBottom = sInvY + sInvHeight;
  aRect.iRight = sInvX + sInvWidth;

  RestrictMouseCursor(&aRect);
}

static void DeleteItemStackPopup();

static void EndItemStackPopupWithItemInHand() {
  if (gpItemPointer != NULL) {
    DeleteItemStackPopup();
  }
}

void RenderItemStackPopup(BOOLEAN fFullRender) {
  if (gfInItemStackPopup) {
    // Disable all faces
    SetAllAutoFacesInactive();

    // Shadow Area
    if (fFullRender) {
      FRAME_BUFFER->ShadowRect(gsItemPopupInvX, gsItemPopupInvY,
                               gsItemPopupInvX + gsItemPopupInvWidth,
                               gsItemPopupInvY + gsItemPopupInvHeight);
    }
  }
  // TAKE A LOOK AT THE VIDEO OBJECT SIZE ( ONE OF TWO SIZES ) AND CENTER!
  ETRLEObject const &pTrav = guiItemPopupBoxes->SubregionProperties(0);
  uint32_t const usWidth = pTrav.usWidth;
  uint32_t const usHeight = pTrav.usHeight;

  for (uint32_t cnt = 0; cnt < gubNumItemPopups; cnt++) {
    uint32_t row = cnt / MAX_STACK_POPUP_WIDTH;
    uint32_t col = cnt % MAX_STACK_POPUP_WIDTH;

    BltVideoObject(FRAME_BUFFER, guiItemPopupBoxes, 0, gsItemPopupX + col * usWidth,
                   gsItemPopupY + row * usHeight);

    if (cnt < gpItemPopupObject->ubNumberOfObjects) {
      int16_t sX = gsItemPopupX + col * usWidth + 11;
      int16_t sY = gsItemPopupY + row * usHeight + 3;

      INVRenderItem(FRAME_BUFFER, NULL, *gpItemPopupObject, sX, sY, 29, 23, DIRTYLEVEL2,
                    RENDER_ITEM_NOSTATUS, SGP_TRANSPARENT);

      // Do status bar here...
      int16_t sNewX = gsItemPopupX + col * usWidth + 7;
      int16_t sNewY = gsItemPopupY + row * usHeight + INV_BAR_DY + 3;
      DrawItemUIBarEx(*gpItemPopupObject, cnt, sNewX, sNewY, ITEM_BAR_HEIGHT,
                      Get16BPPColor(STATUS_BAR), Get16BPPColor(STATUS_BAR_SHADOW), FRAME_BUFFER);
    }
  }

  // RestoreExternBackgroundRect( gsItemPopupInvX, gsItemPopupInvY,
  // gsItemPopupInvWidth, gsItemPopupInvHeight );
  InvalidateRegion(gsItemPopupInvX, gsItemPopupInvY, gsItemPopupInvX + gsItemPopupInvWidth,
                   gsItemPopupInvY + gsItemPopupInvHeight);
}

static void DeleteItemStackPopup() {
  int32_t cnt;

  DeleteVideoObject(guiItemPopupBoxes);

  MSYS_RemoveRegion(&gItemPopupRegion);

  gfInItemStackPopup = FALSE;

  for (cnt = 0; cnt < gubNumItemPopups; cnt++) {
    MSYS_RemoveRegion(&gItemPopupRegions[cnt]);
  }

  fInterfacePanelDirty = DIRTYLEVEL2;

  if (guiCurrentItemDescriptionScreen != MAP_SCREEN) {
    EnableSMPanelButtons(TRUE, FALSE);
  }

  FreeMouseCursor();
}

void InitKeyRingPopup(SOLDIERTYPE *const pSoldier, int16_t const sInvX, int16_t const sInvY,
                      int16_t const sInvWidth, int16_t const sInvHeight) {
  SGPRect aRect;
  int16_t sKeyRingItemWidth = 0;
  int16_t sOffSetY = 0, sOffSetX = 0;

  if (guiCurrentScreen == MAP_SCREEN) {
    gsKeyRingPopupInvX = 0;
    sKeyRingItemWidth = MAP_KEY_RING_ROW_WIDTH;
    sOffSetX = 40;
    sOffSetY = 15;
  } else {
    // Set some globals
    gsKeyRingPopupInvX = sInvX + TACTICAL_INVENTORY_KEYRING_GRAPHIC_OFFSET_X;
    sKeyRingItemWidth = KEY_RING_ROW_WIDTH;
    sOffSetY = 8;
  }

  gsKeyRingPopupInvY = sInvY;
  gsKeyRingPopupInvWidth = sInvWidth;
  gsKeyRingPopupInvHeight = sInvHeight;

  gpItemPopupSoldier = pSoldier;

  // Load graphics
  guiItemPopupBoxes = AddVideoObjectFromFile(INTERFACEDIR "/extra_inventory.sti");

  // Get size
  ETRLEObject const &pTrav = guiItemPopupBoxes->SubregionProperties(0);
  uint16_t const usPopupWidth = pTrav.usWidth;
  uint16_t const usPopupHeight = pTrav.usHeight;

  for (int32_t cnt = 0; cnt < NUMBER_KEYS_ON_KEYRING; cnt++) {
    // Build a mouse region here that is over any others.....
    MSYS_DefineRegion(
        &gKeyRingRegions[cnt],
        gsKeyRingPopupInvX + (cnt % sKeyRingItemWidth * usPopupWidth) + sOffSetX,  // top left
        sInvY + (cnt / sKeyRingItemWidth * usPopupHeight) + sOffSetY,              // top right
        gsKeyRingPopupInvX + (cnt % sKeyRingItemWidth + 1) * usPopupWidth +
            sOffSetX,                                                      // bottom left
        sInvY + (cnt / sKeyRingItemWidth + 1) * usPopupHeight + sOffSetY,  // bottom right
        MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, KeyRingSlotInvClickCallback);
    MSYS_SetRegionUserData(&gKeyRingRegions[cnt], 0, cnt);
    // gfItemPopupRegionCallbackEndFix = FALSE;
  }

  // Build a mouse region here that is over any others.....
  MSYS_DefineRegion(&gItemPopupRegion, sInvX, sInvY, sInvX + sInvWidth, sInvY + sInvHeight,
                    MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK,
                    ItemPopupFullRegionCallback);

  // Disable all faces
  SetAllAutoFacesInactive();

  fInterfacePanelDirty = DIRTYLEVEL2;

  if (guiCurrentItemDescriptionScreen != MAP_SCREEN) {
    EnableSMPanelButtons(FALSE, FALSE);
  }

  gfInKeyRingPopup = TRUE;

  // Reserict mouse cursor to panel
  aRect.iTop = sInvY;
  aRect.iLeft = sInvX;
  aRect.iBottom = sInvY + sInvHeight;
  aRect.iRight = sInvX + sInvWidth;

  RestrictMouseCursor(&aRect);
}

void RenderKeyRingPopup(const BOOLEAN fFullRender) {
  const int16_t dx = gsKeyRingPopupInvX;
  const int16_t dy = gsKeyRingPopupInvY;

  if (gfInKeyRingPopup) {
    SetAllAutoFacesInactive();

    if (fFullRender) {
      FRAME_BUFFER->ShadowRect(0, dy, dx + gsKeyRingPopupInvWidth, dy + gsKeyRingPopupInvHeight);
    }
  }

  OBJECTTYPE o;
  memset(&o, 0, sizeof(o));
  o.bStatus[0] = 100;

  ETRLEObject const &pTrav = guiItemPopupBoxes->SubregionProperties(0);
  uint32_t const box_w = pTrav.usWidth;
  uint32_t const box_h = pTrav.usHeight;

  int16_t offset_x;
  int16_t offset_y;
  int16_t key_ring_cols;
  if (guiCurrentScreen == MAP_SCREEN) {
    offset_x = 40;
    offset_y = 15;
    key_ring_cols = MAP_KEY_RING_ROW_WIDTH;
  } else {
    offset_x = 0;
    offset_y = 8;
    key_ring_cols = KEY_RING_ROW_WIDTH;
  }

  const KEY_ON_RING *const key_ring = gpItemPopupSoldier->pKeyRing;
  for (uint32_t i = 0; i < NUMBER_KEYS_ON_KEYRING; ++i) {
    const uint32_t x = dx + offset_x + i % key_ring_cols * box_w;
    const uint32_t y = dy + offset_y + i / key_ring_cols * box_h;

    BltVideoObject(FRAME_BUFFER, guiItemPopupBoxes, 0, x, y);

    const KEY_ON_RING *const key = &key_ring[i];
    if (key->ubKeyID == INVALID_KEY_NUMBER || key->ubNumber == 0) continue;

    o.ubNumberOfObjects = key->ubNumber;
    o.usItem = FIRST_KEY + LockTable[key->ubKeyID].usKeyItem;

    DrawItemUIBarEx(o, 0, x + 7, y + 24, ITEM_BAR_HEIGHT, Get16BPPColor(STATUS_BAR),
                    Get16BPPColor(STATUS_BAR_SHADOW), FRAME_BUFFER);
    INVRenderItem(FRAME_BUFFER, NULL, o, x + 8, y, box_w - 8, box_h - 2, DIRTYLEVEL2, 0,
                  SGP_TRANSPARENT);
  }

  InvalidateRegion(dx, dy, dx + gsKeyRingPopupInvWidth, dy + gsKeyRingPopupInvHeight);
}

void DeleteKeyRingPopup() {
  if (!gfInKeyRingPopup) return;

  DeleteVideoObject(guiItemPopupBoxes);

  MSYS_RemoveRegion(&gItemPopupRegion);

  gfInKeyRingPopup = FALSE;

  for (int32_t i = 0; i < NUMBER_KEYS_ON_KEYRING; i++) {
    MSYS_RemoveRegion(&gKeyRingRegions[i]);
  }

  fInterfacePanelDirty = DIRTYLEVEL2;

  if (guiCurrentItemDescriptionScreen != MAP_SCREEN) {
    EnableSMPanelButtons(TRUE, FALSE);
  }

  FreeMouseCursor();
}

SGPVObject const &GetInterfaceGraphicForItem(INVTYPE const &item) {
  // CHECK SUBCLASS
  switch (item.ubGraphicType) {
    case 0:
      return *guiGUNSM;
    case 1:
      return *guiP1ITEMS;
    case 2:
      return *guiP2ITEMS;
    default:
      return *guiP3ITEMS;
  }
}

uint16_t GetTileGraphicForItem(INVTYPE const &item) {
  uint32_t Type;
  switch (item.ubGraphicType) {
    case 0:
      Type = GUNS;
      break;
    case 1:
      Type = P1ITEMS;
      break;
    case 2:
      Type = P2ITEMS;
      break;
    default:
      Type = P3ITEMS;
      break;
  }
  return GetTileIndexFromTypeSubIndex(Type, item.ubGraphicNum + 1);
}

SGPVObject *LoadTileGraphicForItem(const INVTYPE &item) {
  const char *Prefix;
  switch (item.ubGraphicType) {
    case 0:
      Prefix = "gun";
      break;
    case 1:
      Prefix = "p1item";
      break;
    case 2:
      Prefix = "p2item";
      break;
    default:
      Prefix = "p3item";
      break;
  }

  // Load item
  SGPFILENAME ImageFile;
  sprintf(ImageFile, BIGITEMSDIR "/%s%02d.sti", Prefix, item.ubGraphicNum);
  return AddVideoObjectFromFile(ImageFile);
}

static void ItemDescCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  static BOOLEAN fRightDown = FALSE, fLeftDown = FALSE;

  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    fLeftDown = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (fLeftDown) {
      fLeftDown = FALSE;

      // Only exit the screen if we are NOT in the money interface.  Only the
      // DONE button should exit the money interface.
      if (gpItemDescObject->usItem != MONEY) {
        DeleteItemDescriptionBox();
      }
    }
  } else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    fRightDown = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    if (fRightDown) {
      fRightDown = FALSE;

      // Only exit the screen if we are NOT in the money interface.  Only the
      // DONE button should exit the money interface.
      //			if( gpItemDescObject->usItem != MONEY )
      { DeleteItemDescriptionBox(); }
    }
  }
}

static void RemoveMoney();

static void ItemDescDoneButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (gpItemDescObject->usItem == MONEY) RemoveMoney();
    DeleteItemDescriptionBox();
  }

  if (reason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    DeleteItemDescriptionBox();
  }
}

static void ItemPopupRegionCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  uint32_t uiItemPos;

  uiItemPos = MSYS_GetRegionUserData(pRegion, 0);

  // TO ALLOW ME TO DELETE REGIONS IN CALLBACKS!
  if (gfItemPopupRegionCallbackEndFix) {
    return;
  }

  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    // If one in our hand, place it
    if (gpItemPointer != NULL) {
      if (!PlaceObjectAtObjectIndex(gpItemPointer, gpItemPopupObject, (uint8_t)uiItemPos)) {
        if (fInMapMode) {
          MAPEndItemPointer();
        } else {
          gpItemPointer = NULL;
          gSMPanelRegion.ChangeCursor(CURSOR_NORMAL);
          SetCurrentCursorFromDatabase(CURSOR_NORMAL);

          if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
            memset(&gMoveingItem, 0, sizeof(INVENTORY_IN_SLOT));
            SetSkiCursor(CURSOR_NORMAL);
          }
        }

        // re-evaluate repairs
        gfReEvaluateEveryonesNothingToDo = TRUE;
      }

      // Dirty interface
      // fInterfacePanelDirty = DIRTYLEVEL2;
      // RenderItemStackPopup( FALSE );
    } else {
      if (uiItemPos < gpItemPopupObject->ubNumberOfObjects) {
        // Here, grab an item and put in cursor to swap
        // RemoveObjFrom( OBJECTTYPE * pObj, uint8_t ubRemoveIndex )
        GetObjFrom(gpItemPopupObject, (uint8_t)uiItemPos, &gItemPointer);

        if (fInMapMode) {
          // pick it up
          InternalMAPBeginItemPointer(gpItemPopupSoldier);
        } else {
          SetItemPointer(&gItemPointer, gpItemPopupSoldier);
        }

        // if we are in the shop keeper interface
        if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
          // pick up stacked item into cursor and try to sell it ( unless CTRL
          // is held down )
          BeginSkiItemPointer(PLAYERS_INVENTORY, -1, !IsKeyDown(CTRL));

          // if we've just removed the last one there
          if (gpItemPopupObject->ubNumberOfObjects == 0) {
            // we must immediately get out of item stack popup, because the item
            // has been deleted (memset to 0), and errors like a right bringing
            // up an item description for item 0 could happen then.  ARM.
            DeleteItemStackPopup();
          }
        }

        // re-evaluate repairs
        gfReEvaluateEveryonesNothingToDo = TRUE;

        // Dirty interface
        // RenderItemStackPopup( FALSE );
        // fInterfacePanelDirty = DIRTYLEVEL2;
      }
    }

    UpdateItemHatches();
  } else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    // Get Description....
    // Some global stuff here - for esc, etc
    // Remove
    gfItemPopupRegionCallbackEndFix = TRUE;

    DeleteItemStackPopup();

    if (!InItemDescriptionBox()) {
      // RESTORE BACKGROUND
      RestoreExternBackgroundRect(gsItemPopupInvX, gsItemPopupInvY, gsItemPopupInvWidth,
                                  gsItemPopupInvHeight);
      if (guiCurrentItemDescriptionScreen == MAP_SCREEN) {
        MAPInternalInitItemDescriptionBox(gpItemPopupObject, (uint8_t)uiItemPos,
                                          gpItemPopupSoldier);
      } else {
        InternalInitItemDescriptionBox(gpItemPopupObject, (int16_t)ITEMDESC_START_X,
                                       (int16_t)ITEMDESC_START_Y, (uint8_t)uiItemPos,
                                       gpItemPopupSoldier);
      }
    }
  }
}

static void ItemPopupFullRegionCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (InItemStackPopup()) {
      // End stack popup and retain pointer
      EndItemStackPopupWithItemInHand();
    } else if (InKeyRingPopup()) {
      // end pop up with key in hand
      DeleteKeyRingPopup();
      fTeamPanelDirty = TRUE;
    }
  } else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    if (InItemStackPopup()) {
      DeleteItemStackPopup();
      fTeamPanelDirty = TRUE;
    } else {
      DeleteKeyRingPopup();
      fTeamPanelDirty = TRUE;
    }
  }
}

#define NUM_PICKUP_SLOTS 6

struct ITEM_PICKUP_MENU_STRUCT {
  ITEM_POOL *pItemPool;
  int16_t sX;
  int16_t sY;
  int16_t sWidth;
  int16_t sHeight;
  int8_t bScrollPage;
  int32_t ubScrollAnchor;
  int32_t ubTotalItems;
  int32_t bCurSelect;
  uint8_t bNumSlotsPerPage;
  SGPVObject *uiPanelVo;
  BUTTON_PICS *iUpButtonImages;
  BUTTON_PICS *iDownButtonImages;
  BUTTON_PICS *iAllButtonImages;
  BUTTON_PICS *iCancelButtonImages;
  BUTTON_PICS *iOKButtonImages;
  GUIButtonRef iUpButton;
  GUIButtonRef iDownButton;
  GUIButtonRef iAllButton;
  GUIButtonRef iOKButton;
  GUIButtonRef iCancelButton;
  BOOLEAN fDirtyLevel;
  BOOLEAN fHandled;
  int16_t sGridNo;
  int8_t bZLevel;
  int16_t sButtomPanelStartY;
  SOLDIERTYPE *pSoldier;
  int32_t items[NUM_PICKUP_SLOTS];
  MOUSE_REGION Regions[NUM_PICKUP_SLOTS];
  MOUSE_REGION BackRegions;
  MOUSE_REGION BackRegion;
  BOOLEAN *pfSelectedArray;
  OBJECTTYPE CompAmmoObject;
  BOOLEAN fAllSelected;
};

#define ITEMPICK_UP_X 55
#define ITEMPICK_UP_Y 5
#define ITEMPICK_DOWN_X 111
#define ITEMPICK_DOWN_Y 5
#define ITEMPICK_ALL_X 79
#define ITEMPICK_ALL_Y 6
#define ITEMPICK_OK_X 16
#define ITEMPICK_OK_Y 6
#define ITEMPICK_CANCEL_X 141
#define ITEMPICK_CANCEL_Y 6

#define ITEMPICK_START_X_OFFSET 10

#define ITEMPICK_GRAPHIC_X 10
#define ITEMPICK_GRAPHIC_Y 12
#define ITEMPICK_GRAPHIC_YSPACE 26

#define ITEMPICK_TEXT_X 56
#define ITEMPICK_TEXT_Y 22
#define ITEMPICK_TEXT_YSPACE 26
#define ITEMPICK_TEXT_WIDTH 109

static ITEM_PICKUP_MENU_STRUCT gItemPickupMenu;
BOOLEAN gfInItemPickupMenu = FALSE;

// STUFF FOR POPUP ITEM INFO BOX
void SetItemPickupMenuDirty(BOOLEAN fDirtyLevel) { gItemPickupMenu.fDirtyLevel = fDirtyLevel; }

static void CalculateItemPickupMenuDimensions();
static void ItemPickMenuMouseClickCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void ItemPickMenuMouseMoveCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void ItemPickupAll(GUI_BUTTON *btn, int32_t reason);
static void ItemPickupCancel(GUI_BUTTON *btn, int32_t reason);
static void ItemPickupOK(GUI_BUTTON *btn, int32_t reason);
static void ItemPickupScrollDown(GUI_BUTTON *btn, int32_t reason);
static void ItemPickupScrollUp(GUI_BUTTON *btn, int32_t reason);
static void SetupPickupPage(int8_t bPage);

void InitializeItemPickupMenu(SOLDIERTYPE *const pSoldier, int16_t const sGridNo,
                              ITEM_POOL *const pItemPool, int8_t const bZLevel) {
  EraseInterfaceMenus(TRUE);
  LocateSoldier(pSoldier, FALSE);

  ITEM_PICKUP_MENU_STRUCT &menu = gItemPickupMenu;
  memset(&menu, 0, sizeof(menu));
  menu.pItemPool = pItemPool;

  InterruptTime();
  PauseGame();
  LockPauseState(LOCK_PAUSE_18);
  PauseTime(TRUE);

  // Alrighty, cancel lock UI if we havn't done so already
  UnSetUIBusy(pSoldier);

  // Change to INV panel if not there already...
  SetNewPanel(pSoldier);

  // Determine total #
  int32_t cnt = 0;
  for (ITEM_POOL *i = pItemPool; i; i = i->pNext) {
    if (!ItemPoolOKForDisplay(i, bZLevel)) continue;
    ++cnt;
  }
  menu.ubTotalItems = (uint8_t)cnt;

  // Determine # of slots per page
  menu.bNumSlotsPerPage =
      menu.ubTotalItems < NUM_PICKUP_SLOTS ? menu.ubTotalItems : NUM_PICKUP_SLOTS;

  menu.uiPanelVo = AddVideoObjectFromFile(INTERFACEDIR "/itembox.sti");

  menu.pfSelectedArray = MALLOCNZ(BOOLEAN, menu.ubTotalItems);

  CalculateItemPickupMenuDimensions();

  // First get mouse xy screen location
  int16_t sX = gusMouseXPos;
  int16_t sY = gusMouseYPos;

  // CHECK FOR LEFT/RIGHT
  if (sX + menu.sWidth > SCREEN_WIDTH) {
    sX = SCREEN_WIDTH - menu.sWidth - ITEMPICK_START_X_OFFSET;
  } else {
    sX = sX + ITEMPICK_START_X_OFFSET;
  }

  // Now check for top
  // Center in the y
  int16_t const sCenterYVal = menu.sHeight / 2;

  sY -= sCenterYVal;
  if (sY < gsVIEWPORT_WINDOW_START_Y) {
    sY = gsVIEWPORT_WINDOW_START_Y;
  }

  // Check for bottom
  if (sY + menu.sHeight > gsVIEWPORT_WINDOW_END_Y) {
    sY = gsVIEWPORT_WINDOW_END_Y - menu.sHeight;
  }

  menu.sX = sX;
  menu.sY = sY;
  menu.bCurSelect = 0;
  menu.pSoldier = pSoldier;
  menu.fHandled = FALSE;
  menu.sGridNo = sGridNo;
  menu.bZLevel = bZLevel;
  menu.fAllSelected = FALSE;

  // Load images for buttons
  BUTTON_PICS *const pics = LoadButtonImage(INTERFACEDIR "/itembox.sti", 5, 10);
  menu.iUpButtonImages = pics;
  menu.iDownButtonImages = UseLoadedButtonImage(pics, 7, 12);
  menu.iAllButtonImages = UseLoadedButtonImage(pics, 6, 11);
  menu.iCancelButtonImages = UseLoadedButtonImage(pics, 8, 13);
  menu.iOKButtonImages = UseLoadedButtonImage(pics, 4, 9);

  // Build a mouse region here that is over any others.....
  MSYS_DefineRegion(&menu.BackRegion, 532, 367, SCREEN_WIDTH, SCREEN_HEIGHT, MSYS_PRIORITY_HIGHEST,
                    CURSOR_NORMAL, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

  // Build a mouse region here that is over any others.....
  MSYS_DefineRegion(&menu.BackRegions, sX, sY, menu.sX + menu.sWidth, sY + menu.sHeight,
                    MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

  int16_t const by = sY + menu.sButtomPanelStartY;

  // Create buttons
  if (menu.bNumSlotsPerPage == NUM_PICKUP_SLOTS && menu.ubTotalItems > NUM_PICKUP_SLOTS) {
    menu.iUpButton = QuickCreateButton(menu.iUpButtonImages, sX + ITEMPICK_UP_X, by + ITEMPICK_UP_Y,
                                       MSYS_PRIORITY_HIGHEST, ItemPickupScrollUp);
    menu.iUpButton->SetFastHelpText(ItemPickupHelpPopup[1]);

    menu.iDownButton =
        QuickCreateButton(menu.iDownButtonImages, sX + ITEMPICK_DOWN_X, by + ITEMPICK_DOWN_Y,
                          MSYS_PRIORITY_HIGHEST, ItemPickupScrollDown);
    menu.iDownButton->SetFastHelpText(ItemPickupHelpPopup[3]);
  }

  menu.iOKButton = QuickCreateButton(menu.iOKButtonImages, sX + ITEMPICK_OK_X, by + ITEMPICK_OK_Y,
                                     MSYS_PRIORITY_HIGHEST, ItemPickupOK);
  menu.iOKButton->SetFastHelpText(ItemPickupHelpPopup[0]);

  menu.iAllButton = QuickCreateButton(menu.iAllButtonImages, sX + ITEMPICK_ALL_X,
                                      by + ITEMPICK_ALL_Y, MSYS_PRIORITY_HIGHEST, ItemPickupAll);
  menu.iAllButton->SetFastHelpText(ItemPickupHelpPopup[2]);

  menu.iCancelButton =
      QuickCreateButton(menu.iCancelButtonImages, sX + ITEMPICK_CANCEL_X, by + ITEMPICK_CANCEL_Y,
                        MSYS_PRIORITY_HIGHEST, ItemPickupCancel);
  menu.iCancelButton->SetFastHelpText(ItemPickupHelpPopup[4]);

  DisableButton(menu.iOKButton);

  // Create regions
  int16_t const sCenX = sX;
  int16_t sCenY = sY + ITEMPICK_GRAPHIC_Y;
  for (int32_t i = 0; i < menu.bNumSlotsPerPage; ++i) {
    MOUSE_REGION *const r = &menu.Regions[i];
    MSYS_DefineRegion(r, sCenX, sCenY + 1, sCenX + menu.sWidth, sCenY + ITEMPICK_GRAPHIC_YSPACE,
                      MSYS_PRIORITY_HIGHEST, CURSOR_NORMAL, ItemPickMenuMouseMoveCallback,
                      ItemPickMenuMouseClickCallback);
    MSYS_SetRegionUserData(r, 0, i);

    sCenY += ITEMPICK_GRAPHIC_YSPACE;
  }

  SetupPickupPage(0);

  gfInItemPickupMenu = TRUE;
  gfIgnoreScrolling = TRUE;

  HandleAnyMercInSquadHasCompatibleStuff(NULL);
  gSelectSMPanelToMerc = pSoldier;
  ReEvaluateDisabledINVPanelButtons();
  DisableTacticalTeamPanelButtons(TRUE);
}

static void SetupPickupPage(int8_t bPage) {
  int32_t cnt, iStart, iEnd;
  ITEM_POOL *pTempItemPool;
  int16_t sValue;

  // Reset page slots
  FOR_EACH(int32_t, i, gItemPickupMenu.items) { *i = -1; }

  // Get lower bound
  iStart = bPage * NUM_PICKUP_SLOTS;
  if (iStart > gItemPickupMenu.ubTotalItems) {
    return;
  }

  iEnd = iStart + NUM_PICKUP_SLOTS;
  if (iEnd >= gItemPickupMenu.ubTotalItems) {
    iEnd = gItemPickupMenu.ubTotalItems;
  }

  // Setup slots!
  // These slots contain an inventory pool pointer for each slot...
  pTempItemPool = gItemPickupMenu.pItemPool;

  // ATE: Patch fix here for crash :(
  // Clear help text!
  for (cnt = 0; cnt < NUM_PICKUP_SLOTS; cnt++) {
    gItemPickupMenu.Regions[cnt].SetFastHelpText(L"");
  }

  for (cnt = 0; cnt < iEnd;) {
    // Move to the closest one that can be displayed....
    while (!ItemPoolOKForDisplay(pTempItemPool, gItemPickupMenu.bZLevel)) {
      pTempItemPool = pTempItemPool->pNext;
    }

    if (cnt >= iStart) {
      int32_t const item = pTempItemPool->iItemIndex;
      gItemPickupMenu.items[cnt - iStart] = item;

      OBJECTTYPE const &o = GetWorldItem(item).o;

      sValue = o.bStatus[0];

      // Adjust for ammo, other thingys..
      wchar_t pStr[200];
      if (Item[o.usItem].usItemClass & IC_AMMO || Item[o.usItem].usItemClass & IC_KEY) {
        swprintf(pStr, lengthof(pStr), L"");
      } else {
        swprintf(pStr, lengthof(pStr), L"%d%%", sValue);
      }

      gItemPickupMenu.Regions[cnt - iStart].SetFastHelpText(pStr);
    }

    cnt++;

    pTempItemPool = pTempItemPool->pNext;
  }

  gItemPickupMenu.bScrollPage = bPage;
  gItemPickupMenu.ubScrollAnchor = (uint8_t)iStart;

  if (gItemPickupMenu.bNumSlotsPerPage == NUM_PICKUP_SLOTS &&
      gItemPickupMenu.ubTotalItems > NUM_PICKUP_SLOTS) {
    // Setup enabled/disabled buttons
    EnableButton(gItemPickupMenu.iUpButton, bPage > 0);
    // Setup enabled/disabled buttons
    EnableButton(gItemPickupMenu.iDownButton, iEnd < gItemPickupMenu.ubTotalItems);
  }
  SetItemPickupMenuDirty(DIRTYLEVEL2);
}

static void CalculateItemPickupMenuDimensions() {
  // Build background
  int16_t sY = 0;

  for (int32_t cnt = 0; cnt < gItemPickupMenu.bNumSlotsPerPage; cnt++) {
    // Add height of object
    uint16_t usSubRegion = (cnt == 0 ? 0 : 1);
    ETRLEObject const &ETRLEProps = gItemPickupMenu.uiPanelVo->SubregionProperties(usSubRegion);
    sY += ETRLEProps.usHeight;
  }
  gItemPickupMenu.sButtomPanelStartY = sY;

  // Do end
  ETRLEObject const &ETRLEProps = gItemPickupMenu.uiPanelVo->SubregionProperties(2);
  sY += ETRLEProps.usHeight;

  // Set height, width
  gItemPickupMenu.sHeight = sY;
  gItemPickupMenu.sWidth = ETRLEProps.usWidth;
}

void RenderItemPickupMenu() {
  wchar_t pStr[100];

  if (!gfInItemPickupMenu) return;

  ITEM_PICKUP_MENU_STRUCT &menu = gItemPickupMenu;
  if (menu.fDirtyLevel != DIRTYLEVEL2) return;

  MarkButtonsDirty();

  // Build background
  int16_t sX = menu.sX;
  int16_t sY = menu.sY;

  for (int32_t cnt = 0; cnt < menu.bNumSlotsPerPage; ++cnt) {
    uint16_t const usSubRegion = (cnt == 0 ? 0 : 1);

    BltVideoObject(FRAME_BUFFER, menu.uiPanelVo, usSubRegion, sX, sY);

    // Add height of object
    ETRLEObject const &ETRLEProps = menu.uiPanelVo->SubregionProperties(usSubRegion);
    sY += ETRLEProps.usHeight;
  }

  // Do end
  uint16_t const gfx =
      menu.bNumSlotsPerPage == NUM_PICKUP_SLOTS && menu.ubTotalItems > NUM_PICKUP_SLOTS ? 2 : 3;
  BltVideoObject(FRAME_BUFFER, menu.uiPanelVo, gfx, sX, sY);

  // Render items....
  sX = menu.sX + ITEMPICK_GRAPHIC_X;
  sY = menu.sY + ITEMPICK_GRAPHIC_Y;

  SetFont(ITEMDESC_FONT);
  SetFontBackground(FONT_MCOLOR_BLACK);
  SetFontShadow(ITEMDESC_FONTSHADOW2);

  {
    SGPVSurface::Lock l(FRAME_BUFFER);
    uint16_t *const pDestBuf = l.Buffer<uint16_t>();
    uint32_t const uiDestPitchBYTES = l.Pitch();

    uint16_t const outline_col = Get16BPPColor(FROMRGB(255, 255, 0));
    for (int32_t cnt = 0; cnt < menu.bNumSlotsPerPage; ++cnt) {
      int32_t const world_item = menu.items[cnt];
      if (world_item == -1) continue;

      // Get item to render
      OBJECTTYPE const &o = GetWorldItem(world_item).o;
      INVTYPE const &item = Item[o.usItem];

      uint16_t const usItemTileIndex = GetTileGraphicForItem(item);
      TILE_ELEMENT const *const te = &gTileDatabase[usItemTileIndex];

      // ATE: Adjust to basic shade.....
      te->hTileSurface->CurrentShade(4);

      uint16_t const outline =
          menu.pfSelectedArray[cnt + menu.ubScrollAnchor] ? outline_col : SGP_TRANSPARENT;
      Blt8BPPDataTo16BPPBufferOutline(pDestBuf, uiDestPitchBYTES, te->hTileSurface, sX, sY,
                                      te->usRegionIndex, outline);

      if (o.ubNumberOfObjects > 1) {
        SetFontAttributes(ITEM_FONT, FONT_GRAY4);

        swprintf(pStr, lengthof(pStr), L"%d", o.ubNumberOfObjects);

        int16_t sFontX;
        int16_t sFontY;
        FindFontRightCoordinates(sX - 4, sY + 14, 42, 1, pStr, ITEM_FONT, &sFontX, &sFontY);
        MPrintBuffer(pDestBuf, uiDestPitchBYTES, sFontX, sFontY, pStr);
        SetFont(ITEMDESC_FONT);
      }

      if (ItemHasAttachments(o)) {  // Render attachment symbols
        SetFontForeground(FONT_GREEN);
        SetFontShadow(DEFAULT_SHADOW);
        wchar_t const *const AttachMarker = L"*";
        uint16_t const uiStringLength = StringPixLength(AttachMarker, ITEM_FONT);
        int16_t const sNewX = sX + 43 - uiStringLength - 4;
        int16_t const sNewY = sY + 2;
        MPrintBuffer(pDestBuf, uiDestPitchBYTES, sNewX, sNewY, AttachMarker);
      }

      if (menu.bCurSelect == cnt + menu.ubScrollAnchor) {
        SetFontForeground(FONT_WHITE);
        SetFontShadow(DEFAULT_SHADOW);
      } else {
        SetFontForeground(FONT_BLACK);
        SetFontShadow(ITEMDESC_FONTSHADOW2);
      }

      // Render name
      if (item.usItemClass == IC_MONEY) {
        wchar_t pStr2[20];
        SPrintMoney(pStr2, o.uiMoneyAmount);
        swprintf(pStr, lengthof(pStr), L"%ls (%ls)", ItemNames[o.usItem], pStr2);
      } else {
        wcsncpy(pStr, ShortItemNames[o.usItem], lengthof(pStr));
      }
      int16_t sFontX;
      int16_t sFontY;
      int16_t const x = ITEMPICK_TEXT_X + menu.sX;
      int16_t const y = ITEMPICK_TEXT_Y + menu.sY + ITEMPICK_TEXT_YSPACE * cnt;
      FindFontCenterCoordinates(x, y, ITEMPICK_TEXT_WIDTH, 1, pStr, ITEMDESC_FONT, &sFontX,
                                &sFontY);
      MPrintBuffer(pDestBuf, uiDestPitchBYTES, sFontX, sFontY, pStr);

      sY += ITEMPICK_GRAPHIC_YSPACE;
    }
  }

  SetFontShadow(DEFAULT_SHADOW);
  InvalidateRegion(menu.sX, menu.sY, menu.sX + menu.sWidth, menu.sY + menu.sHeight);
  menu.fDirtyLevel = 0;
}

void RemoveItemPickupMenu() {
  int32_t cnt;

  if (gfInItemPickupMenu) {
    gfSMDisableForItems = FALSE;

    HandleAnyMercInSquadHasCompatibleStuff(NULL);

    UnLockPauseState();
    UnPauseGame();
    // UnPause timers as well....
    PauseTime(FALSE);

    // Unfreese guy!
    gItemPickupMenu.pSoldier->fPauseAllAnimation = FALSE;

    DeleteVideoObject(gItemPickupMenu.uiPanelVo);

    // Remove buttons
    if (gItemPickupMenu.bNumSlotsPerPage == NUM_PICKUP_SLOTS &&
        gItemPickupMenu.ubTotalItems > NUM_PICKUP_SLOTS) {
      RemoveButton(gItemPickupMenu.iUpButton);
      RemoveButton(gItemPickupMenu.iDownButton);
    }
    RemoveButton(gItemPickupMenu.iAllButton);
    RemoveButton(gItemPickupMenu.iOKButton);
    RemoveButton(gItemPickupMenu.iCancelButton);

    // Remove button images
    UnloadButtonImage(gItemPickupMenu.iUpButtonImages);
    UnloadButtonImage(gItemPickupMenu.iDownButtonImages);
    UnloadButtonImage(gItemPickupMenu.iAllButtonImages);
    UnloadButtonImage(gItemPickupMenu.iCancelButtonImages);
    UnloadButtonImage(gItemPickupMenu.iOKButtonImages);

    MSYS_RemoveRegion(&(gItemPickupMenu.BackRegions));
    MSYS_RemoveRegion(&(gItemPickupMenu.BackRegion));

    // Remove regions
    for (cnt = 0; cnt < gItemPickupMenu.bNumSlotsPerPage; cnt++) {
      MSYS_RemoveRegion(&(gItemPickupMenu.Regions[cnt]));
    }

    // Free selection list...
    MemFree(gItemPickupMenu.pfSelectedArray);
    gItemPickupMenu.pfSelectedArray = NULL;

    // Set cursor back to normal mode...
    guiPendingOverrideEvent = A_CHANGE_TO_MOVE;

    // Rerender world
    SetRenderFlags(RENDER_FLAG_FULL);

    gfInItemPickupMenu = FALSE;

    // gfSMDisableForItems = FALSE;
    EnableSMPanelButtons(TRUE, TRUE);
    gfSMDisableForItems = FALSE;

    fInterfacePanelDirty = DIRTYLEVEL2;

    // Turn off Ignore scrolling
    gfIgnoreScrolling = FALSE;
    DisableTacticalTeamPanelButtons(FALSE);
    gSelectSMPanelToMerc = gpSMCurrentMerc;
  }
}

static void ItemPickupScrollUp(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    SetupPickupPage((uint8_t)(gItemPickupMenu.bScrollPage - 1));
  }
}

static void ItemPickupScrollDown(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    SetupPickupPage((uint8_t)(gItemPickupMenu.bScrollPage + 1));
  }
}

static void ItemPickupAll(GUI_BUTTON *btn, int32_t reason) {
  int32_t cnt;

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gItemPickupMenu.fAllSelected = !gItemPickupMenu.fAllSelected;

    // OK, pickup item....
    // gItemPickupMenu.fHandled = TRUE;
    // Tell our soldier to pickup this item!
    // SoldierGetItemFromWorld( gItemPickupMenu.pSoldier,
    // ITEM_PICKUP_ACTION_ALL, gItemPickupMenu.sGridNo, gItemPickupMenu.bZLevel,
    // NULL );
    for (cnt = 0; cnt < gItemPickupMenu.ubTotalItems; cnt++) {
      gItemPickupMenu.pfSelectedArray[cnt] = gItemPickupMenu.fAllSelected;
    }

    EnableButton(gItemPickupMenu.iOKButton, gItemPickupMenu.fAllSelected);
  }
}

static void ItemPickupOK(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // OK, pickup item....
    gItemPickupMenu.fHandled = TRUE;

    // Tell our soldier to pickup this item!
    SoldierGetItemFromWorld(gItemPickupMenu.pSoldier, ITEM_PICKUP_SELECTION,
                            gItemPickupMenu.sGridNo, gItemPickupMenu.bZLevel,
                            gItemPickupMenu.pfSelectedArray);
  }
}

static void ItemPickupCancel(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // OK, pickup item....
    gItemPickupMenu.fHandled = TRUE;
  }
}

static void ItemPickMenuMouseMoveCallback(MOUSE_REGION *const pRegion, int32_t const iReason) {
  static BOOLEAN bChecked = FALSE;

  if (iReason & MSYS_CALLBACK_REASON_MOVE) {
    uint32_t const uiItemPos = MSYS_GetRegionUserData(pRegion, 0);
    int32_t const bPos = uiItemPos + gItemPickupMenu.ubScrollAnchor;
    if (bPos >= gItemPickupMenu.ubTotalItems) return;

    gItemPickupMenu.bCurSelect = bPos;

    if (bChecked) return;

    // Show compatible ammo
    int32_t const item = gItemPickupMenu.items[uiItemPos];
    OBJECTTYPE const &o = GetWorldItem(item).o;

    gItemPickupMenu.CompAmmoObject = o;

    HandleAnyMercInSquadHasCompatibleStuff(0);  // Turn off first
    InternalHandleCompatibleAmmoUI(gpSMCurrentMerc, &gItemPickupMenu.CompAmmoObject, TRUE);
    HandleAnyMercInSquadHasCompatibleStuff(&o);

    SetItemPickupMenuDirty(DIRTYLEVEL2);

    bChecked = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    gItemPickupMenu.bCurSelect = 255;

    InternalHandleCompatibleAmmoUI(gpSMCurrentMerc, &gItemPickupMenu.CompAmmoObject, FALSE);
    HandleAnyMercInSquadHasCompatibleStuff(NULL);

    SetItemPickupMenuDirty(DIRTYLEVEL2);

    bChecked = FALSE;
  }
}

static void ItemPickMenuMouseClickCallback(MOUSE_REGION *const pRegion, int32_t const iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    int32_t const item_pos = MSYS_GetRegionUserData(pRegion, 0) + gItemPickupMenu.ubScrollAnchor;
    if (item_pos >= gItemPickupMenu.ubTotalItems) return;

    BOOLEAN &selected = gItemPickupMenu.pfSelectedArray[item_pos];
    selected = !selected;

    // Loop through all and set /unset OK
    bool enable = false;
    for (uint8_t i = 0; i < gItemPickupMenu.ubTotalItems; ++i) {
      if (!gItemPickupMenu.pfSelectedArray[i]) continue;
      enable = true;
      break;
    }
    EnableButton(gItemPickupMenu.iOKButton, enable);
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    int8_t const page = gItemPickupMenu.bScrollPage;
    if (page > 0) SetupPickupPage(page - 1);
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    int8_t const page = gItemPickupMenu.bScrollPage;
    if ((page + 1) * NUM_PICKUP_SLOTS < gItemPickupMenu.ubTotalItems) {
      SetupPickupPage(page + 1);
    }
  }
}

BOOLEAN HandleItemPickupMenu() {
  if (!gfInItemPickupMenu) {
    return (FALSE);
  }

  if (gItemPickupMenu.fHandled) {
    RemoveItemPickupMenu();
  }

  return (gItemPickupMenu.fHandled);
}

static void BtnMoneyButtonCallback(GUI_BUTTON *const btn, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    btn->uiFlags |= BUTTON_CLICKED_ON;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    int32_t amount = 0;
    uint8_t const ubButton = btn->GetUserData();
    switch (ubButton) {
      case M_1000:
        amount = 1000;
        break;
      case M_100:
        amount = 100;
        break;
      case M_10:
        amount = 10;
        break;

      case M_DONE:
        RemoveMoney();
        DeleteItemDescriptionBox();
        break;
    }

    if (amount != 0 && gRemoveMoney.uiMoneyRemaining >= amount) {
      if (gfAddingMoneyToMercFromPlayersAccount &&
          gRemoveMoney.uiMoneyRemoving + amount > MAX_MONEY_PER_SLOT) {
        ScreenID const exit_screen =
            guiCurrentScreen == SHOPKEEPER_SCREEN ? SHOPKEEPER_SCREEN : GAME_SCREEN;
        DoMessageBox(MSG_BOX_BASIC_STYLE,
                     gzMoneyWithdrawMessageText[MONEY_TEXT_WITHDRAW_MORE_THEN_MAXIMUM], exit_screen,
                     MSG_BOX_FLAG_OK, NULL, NULL);
        return;
      }

      gRemoveMoney.uiMoneyRemaining -= amount;
      gRemoveMoney.uiMoneyRemoving += amount;

      RenderItemDescriptionBox();
      for (int8_t i = 0; i < MAX_ATTACHMENTS; ++i) {
        MarkAButtonDirty(guiMoneyButtonBtn[i]);
      }
    }
  }

  if (reason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    btn->uiFlags &= ~BUTTON_CLICKED_ON;

    int32_t amount = 0;
    uint8_t const ubButton = btn->GetUserData();
    switch (ubButton) {
      case M_1000:
        amount = 1000;
        break;
      case M_100:
        amount = 100;
        break;
      case M_10:
        amount = 10;
        break;
    }

    if (amount != 0 && gRemoveMoney.uiMoneyRemoving >= amount) {
      gRemoveMoney.uiMoneyRemaining += amount;
      gRemoveMoney.uiMoneyRemoving -= amount;
    }

    RenderItemDescriptionBox();
    for (int8_t i = 0; i < MAX_ATTACHMENTS; ++i) {
      MarkAButtonDirty(guiMoneyButtonBtn[i]);
    }
  }
}

static void RemoveMoney() {
  if (gRemoveMoney.uiMoneyRemoving != 0) {
    // if we are in the shop keeper interface
    if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
      INVENTORY_IN_SLOT InvSlot;

      memset(&InvSlot, 0, sizeof(INVENTORY_IN_SLOT));

      InvSlot.fActive = TRUE;
      InvSlot.sItemIndex = MONEY;
      InvSlot.bSlotIdInOtherLocation = -1;

      // Remove the money from the money in the pocket
      gpItemDescObject->uiMoneyAmount = gRemoveMoney.uiMoneyRemaining;

      // Create an item to get the money that is being removed
      CreateMoney(gRemoveMoney.uiMoneyRemoving, &InvSlot.ItemObject);

      InvSlot.ubIdOfMercWhoOwnsTheItem = gpItemDescSoldier->ubProfile;

      // if we are removing money from the players account
      if (gfAddingMoneyToMercFromPlayersAccount) {
        gpItemDescObject->uiMoneyAmount = gRemoveMoney.uiMoneyRemoving;

        // take the money from the player
        AddTransactionToPlayersBook(TRANSFER_FUNDS_TO_MERC, gpSMCurrentMerc->ubProfile,
                                    GetWorldTotalMin(),
                                    -(int32_t)(gpItemDescObject->uiMoneyAmount));
      }

      gMoveingItem = InvSlot;

      gItemPointer = InvSlot.ItemObject;
      SetItemPointer(&gItemPointer, gpSMCurrentMerc);

      // Set mouse
      SetSkiCursor(EXTERN_CURSOR);

      // Restrict the cursor to the proper area
      RestrictSkiMouseCursor();
    } else {
      CreateMoney(gRemoveMoney.uiMoneyRemoving, &gItemPointer);
      SetItemPointer(&gItemPointer, gpItemDescSoldier);

      // Remove the money from the money in the pocket
      // if we are removing money from the players account
      if (gfAddingMoneyToMercFromPlayersAccount) {
        gpItemDescObject->uiMoneyAmount = gRemoveMoney.uiMoneyRemoving;

        // take the money from the player
        AddTransactionToPlayersBook(TRANSFER_FUNDS_TO_MERC, gpSMCurrentMerc->ubProfile,
                                    GetWorldTotalMin(),
                                    -(int32_t)(gpItemDescObject->uiMoneyAmount));
      } else
        gpItemDescObject->uiMoneyAmount = gRemoveMoney.uiMoneyRemaining;

      if (guiCurrentItemDescriptionScreen == MAP_SCREEN) {
        SetMapCursorItem();
        fTeamPanelDirty = TRUE;
      }
    }
  }

  //	if( gfAddingMoneyToMercFromPlayersAccount )
  //		gfAddingMoneyToMercFromPlayersAccount = FALSE;
}

void GetHelpTextForItem(wchar_t *const dst, size_t const length, OBJECTTYPE const &obj) {
  uint16_t const usItem = obj.usItem;
  if (usItem == MONEY) {
    SPrintMoney(dst, obj.uiMoneyAmount);
  } else if (Item[usItem].usItemClass == IC_MONEY) {  // alternate money like silver or gold
    wchar_t pStr2[20];
    SPrintMoney(pStr2, obj.uiMoneyAmount);
    swprintf(dst, length, L"%ls (%ls)", ItemNames[usItem], pStr2);
  } else if (usItem == NOTHING) {
    wcsncpy(dst, L"", length);
  } else {
    size_t n = swprintf(dst, length, L"%ls", ItemNames[usItem]);
    if (!gGameOptions.fGunNut && Item[usItem].usItemClass == IC_GUN) {
      AmmoKind const calibre = Weapon[usItem].ubCalibre;
      if (calibre != NOAMMO && calibre != AMMOROCKET) {
        n += swprintf(dst + n, length - n, L" (%ls)", AmmoCaliber[calibre]);
      }
    }

    if (wchar_t const *const imprint = GetObjectImprint(obj)) {
      n += swprintf(dst + n, length - n, L" [%ls]", imprint);
    }

    // Add attachment string....
    wchar_t const *const first_prefix = L" (";
    wchar_t const *prefix = first_prefix;
    FOR_EACH(uint16_t const, i, obj.usAttachItem) {
      uint16_t const attachment = *i;
      if (attachment == NOTHING) continue;

      n += swprintf(dst + n, length - n, L"%ls%ls", prefix, ItemNames[attachment]);
      prefix = L",\n";
    }
    if (prefix != first_prefix) {
      n += swprintf(dst + n, length - n, L"%ls", pMessageStrings[MSG_END_ATTACHMENT_LIST]);
    }
  }
}

void CancelItemPointer() {
  // ATE: If we have an item pointer end it!
  if (gpItemPointer != NULL) {
    if (gbItemPointerSrcSlot != NO_SLOT) {
      // Place it back in our hands!
      PlaceObject(gpItemPointerSoldier, gbItemPointerSrcSlot, gpItemPointer);

      // ATE: This could potnetially swap!
      // Make sure # of items is 0, if not, auto place somewhere else...
      if (gpItemPointer->ubNumberOfObjects > 0) {
        if (!AutoPlaceObject(gpItemPointerSoldier, gpItemPointer, FALSE)) {
          // Alright, place of the friggen ground!
          AddItemToPool(gpItemPointerSoldier->sGridNo, gpItemPointer, VISIBLE,
                        gpItemPointerSoldier->bLevel, 0, -1);
          NotifySoldiersToLookforItems();
        }
      }
    } else {
      // We drop it here.....
      AddItemToPool(gpItemPointerSoldier->sGridNo, gpItemPointer, VISIBLE,
                    gpItemPointerSoldier->bLevel, 0, -1);
      NotifySoldiersToLookforItems();
    }
    EndItemPointer();
  }
}

void LoadItemCursorFromSavedGame(HWFILE const f) {
  uint8_t data[44];
  FileRead(f, data, sizeof(data));

  BOOLEAN active;
  SOLDIERTYPE *soldier;
  uint8_t const *d = data;
  d = ExtractObject(d, &gItemPointer);
  EXTR_SOLDIER(d, soldier)
  EXTR_U8(d, gbItemPointerSrcSlot)
  EXTR_BOOL(d, active)
  EXTR_SKIP(d, 5)
  Assert(d == endof(data));

  if (active) {
    SetItemPointer(&gItemPointer, soldier);
    ReEvaluateDisabledINVPanelButtons();
  } else {
    gpItemPointer = 0;
    gpItemPointerSoldier = 0;
  }
}

void SaveItemCursorToSavedGame(HWFILE const f) {
  uint8_t data[44];
  uint8_t *d = data;
  d = InjectObject(d, &gItemPointer);
  INJ_SOLDIER(d, gpItemPointerSoldier)
  INJ_U8(d, gbItemPointerSrcSlot)
  INJ_BOOL(d, gpItemPointer != 0)
  INJ_SKIP(d, 5)
  Assert(d == endof(data));

  FileWrite(f, data, sizeof(data));
}

void UpdateItemHatches() {
  SOLDIERTYPE *pSoldier = NULL;

  if (fInMapMode) {
    if (fShowInventoryFlag) pSoldier = GetSelectedInfoChar();
  } else {
    pSoldier = gpSMCurrentMerc;
  }

  if (pSoldier != NULL) {
    ReevaluateItemHatches(pSoldier, FALSE);
  }
}

void SetMouseCursorFromItem(uint16_t const item_idx) {
  INVTYPE const &item = Item[item_idx];
  SGPVObject const &vo = GetInterfaceGraphicForItem(item);
  SetExternMouseCursor(vo, item.ubGraphicNum);
  SetCurrentCursorFromDatabase(EXTERN_CURSOR);
}

void SetMouseCursorFromCurrentItem() { SetMouseCursorFromItem(gpItemPointer->usItem); }

void SetItemPointer(OBJECTTYPE *const o, SOLDIERTYPE *const s) {
  gpItemPointer = o;
  gpItemPointerSoldier = s;
}

void LoadInterfaceItemsGraphics() {
  guiMapInvSecondHandBlockout = AddVideoObjectFromFile(INTERFACEDIR "/map_inv_2nd_gun_cover.sti");
  guiSecItemHiddenVO = AddVideoObjectFromFile(INTERFACEDIR "/secondary_gun_hidden.sti");
  guiGUNSM = AddVideoObjectFromFile(INTERFACEDIR "/mdguns.sti");       // interface gun pictures
  guiP1ITEMS = AddVideoObjectFromFile(INTERFACEDIR "/mdp1items.sti");  // interface item pictures
  guiP2ITEMS = AddVideoObjectFromFile(INTERFACEDIR "/mdp2items.sti");  // interface item pictures
  guiP3ITEMS = AddVideoObjectFromFile(INTERFACEDIR "/mdp3items.sti");  // interface item pictures

  /* Build a sawtooth black-white-black colour gradient */
  size_t const length = lengthof(us16BPPItemCyclePlacedItemColors);
  for (int32_t i = 0; i != length / 2; ++i) {
    uint32_t const l = 25 * (i + 1);
    uint16_t const c = Get16BPPColor(FROMRGB(l, l, l));
    us16BPPItemCyclePlacedItemColors[i] = c;
    us16BPPItemCyclePlacedItemColors[length - i - 1] = c;
  }
}

void DeleteInterfaceItemsGraphics() {
  DeleteVideoObject(guiMapInvSecondHandBlockout);
  DeleteVideoObject(guiSecItemHiddenVO);
  DeleteVideoObject(guiGUNSM);
  DeleteVideoObject(guiP1ITEMS);
  DeleteVideoObject(guiP2ITEMS);
  DeleteVideoObject(guiP3ITEMS);
}
