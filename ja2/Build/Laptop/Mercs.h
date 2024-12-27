// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __MERCS_H
#define __MERCS_H

#include "SGP/Types.h"

#define MERC_BUTTON_UP_COLOR FONT_MCOLOR_WHITE
#define MERC_BUTTON_DOWN_COLOR FONT_MCOLOR_DKWHITE

#define NUMBER_OF_MERCS 11
#define LAST_MERC_ID 10
#define NUMBER_OF_BAD_MERCS 5

#define MERC_NUM_DAYS_TILL_FIRST_WARNING 7
#define MERC_NUM_DAYS_TILL_ACCOUNT_SUSPENDED 9
#define MERC_NUM_DAYS_TILL_ACCOUNT_INVALID 12

#define MERC_LARRY_ROACHBURN 7

#define DAYS_TIL_M_E_R_C_AVAIL 3

// The players account information for the MERC site
enum {
  MERC_NO_ACCOUNT,
  MERC_ACCOUNT_SUSPENDED,
  MERC_ACCOUNT_INVALID,
  MERC_ACCOUNT_VALID_FIRST_WARNING,
  MERC_ACCOUNT_VALID,
};
// extern	uint8_t			gubPlayersMercAccountStatus;
// extern	uint32_t		guiPlayersMercAccountNumber;

// The video conferencing for the merc page
#define MERC_VIDEO_SPECK_SPEECH_NOT_TALKING 65535
#define MERC_VIDEO_SPECK_HAS_TO_TALK_BUT_QUOTE_NOT_CHOSEN_YET 65534

// used with the gubArrivedFromMercSubSite variable to signify whcih page the
// player came from
enum {
  MERC_CAME_FROM_OTHER_PAGE,
  MERC_CAME_FROM_ACCOUNTS_PAGE,
  MERC_CAME_FROM_HIRE_PAGE,
};

void GameInitMercs();
void EnterMercs();
void ExitMercs();
void HandleMercs();
void RenderMercs();

void InitMercBackGround();
void DrawMecBackGround();
void RemoveMercBackGround();
void DailyUpdateOfMercSite(uint16_t usDate);
uint8_t GetMercIDFromMERCArray(uint8_t ubMercID);
void DisplayTextForSpeckVideoPopUp(const wchar_t *pString);

void EnterInitMercSite();

void GetMercSiteBackOnline();

void DisableMercSiteButton();

extern uint16_t gusMercVideoSpeckSpeech;

extern uint8_t gubArrivedFromMercSubSite;

extern uint8_t gubMercArray[NUMBER_OF_MERCS];
extern uint8_t gubCurMercIndex;

extern BOOLEAN gfJustHiredAMercMerc;

void NewMercsAvailableAtMercSiteCallBack();

void CalcAproximateAmountPaidToSpeck();

#endif
