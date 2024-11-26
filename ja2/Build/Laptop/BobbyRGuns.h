#ifndef __BOBBYRGUNS_H
#define __BOBBYRGUNS_H

#include "SGP/Types.h"

#define BOBBYRDESCFILE BINARYDATADIR "/braydesc.edt"

#define BOBBYR_ITEM_DESC_NAME_SIZE 80
#define BOBBYR_ITEM_DESC_INFO_SIZE 320
#define BOBBYR_ITEM_DESC_FILE_SIZE 400

#define BOBBYR_USED_ITEMS 0xFFFFFFFF

#define BOBBYR_GUNS_BUTTON_FONT FONT10ARIAL
#define BOBBYR_GUNS_TEXT_COLOR_ON FONT_NEARBLACK
#define BOBBYR_GUNS_TEXT_COLOR_OFF FONT_NEARBLACK
// #define		BOBBYR_GUNS_TEXT_COLOR_ON
//  FONT_MCOLOR_DKWHITE #define		BOBBYR_GUNS_TEXT_COLOR_OFF
//  FONT_MCOLOR_WHITE

#define BOBBYR_GUNS_SHADOW_COLOR 169

#define BOBBYR_NO_ITEMS 65535

extern uint16_t gusCurWeaponIndex;
extern uint8_t gubLastGunIndex;

void GameInitBobbyRGuns();
void EnterBobbyRGuns();
void ExitBobbyRGuns();
void RenderBobbyRGuns();

void DisplayBobbyRBrTitle();
void DeleteBobbyBrTitle();
void InitBobbyBrTitle();
void InitBobbyMenuBar();
void DeleteBobbyMenuBar();

// BOOLEAN DisplayWeaponInfo();
void DisplayItemInfo(uint32_t uiItemClass);
void DeleteMouseRegionForBigImage();
void UpdateButtonText(uint32_t uiCurPage);
uint16_t CalcBobbyRayCost(uint16_t usIndex, uint16_t usBobbyIndex, BOOLEAN fUsed);
void SetFirstLastPagesForUsed();
void SetFirstLastPagesForNew(uint32_t uiClass);

#endif
