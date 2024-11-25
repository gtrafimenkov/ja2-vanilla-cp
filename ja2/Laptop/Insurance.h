// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __INSURANCE_H
#define __INSURANCE_H

#include "SGP/Types.h"

#define INS_FONT_COLOR 2
#define INS_FONT_COLOR_RED FONT_MCOLOR_RED
#define INS_FONT_BIG FONT14ARIAL
#define INS_FONT_MED FONT12ARIAL
#define INS_FONT_SMALL FONT10ARIAL

#define INS_FONT_BTN_COLOR FONT_MCOLOR_WHITE
#define INS_FONT_BTN_SHADOW_COLOR 2

#define INS_FONT_SHADOW FONT_MCOLOR_WHITE

#define INSURANCE_BULLET_TEXT_OFFSET_X 21

#define INS_INFO_LEFT_ARROW_BUTTON_X (LAPTOP_SCREEN_UL_X + 71)
#define INS_INFO_RIGHT_ARROW_BUTTON_X (LAPTOP_SCREEN_UL_X + 409)
#define INS_INFO_ARROW_BUTTON_Y (LAPTOP_SCREEN_WEB_UL_Y + 354)

void EnterInsurance();
void ExitInsurance();
void RenderInsurance();

void InitInsuranceDefaults();
void DisplayInsuranceDefaults();
void RemoveInsuranceDefaults();
void DisplaySmallRedLineWithShadow(uint16_t usStartX, uint16_t usStartY, uint16_t EndX,
                                   uint16_t EndY);
void GetInsuranceText(uint8_t ubNumber, wchar_t *pString);

#endif
