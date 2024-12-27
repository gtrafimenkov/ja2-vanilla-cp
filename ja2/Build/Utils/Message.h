// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __MESSAGE_H
#define __MESSAGE_H

#include "SGP/Types.h"

extern uint8_t gubCurrentMapMessageString;
extern BOOLEAN fDisableJustForIan;

#define MSG_INTERFACE 0
#define MSG_DIALOG 1
#define MSG_CHAT 2
#define MSG_DEBUG 3
#define MSG_UI_FEEDBACK 4
#define MSG_ERROR 5
#define MSG_BETAVERSION 6
#define MSG_TESTVERSION 7
#define MSG_SKULL_UI_FEEDBACK 11

// These defines correlate to defines in font.h
#define MSG_FONT_RED FONT_MCOLOR_RED
#define MSG_FONT_YELLOW FONT_MCOLOR_LTYELLOW
#define MSG_FONT_WHITE FONT_MCOLOR_WHITE

// are we allowed to beep on message scroll in tactical
extern BOOLEAN fOkToBeepNewMessage;

void ScreenMsg(uint16_t usColor, uint8_t ubPriority, const wchar_t *pStringA, ...);

// same as screen message, but only display to mapscreen message system, not
// tactical
void MapScreenMessage(uint16_t usColor, uint8_t ubPriority, const wchar_t *pStringA, ...);

void ScrollString();
void DisplayStringsInMapScreenMessageList();

void FreeGlobalMessageList();

uint8_t GetRangeOfMapScreenMessages();

void EnableDisableScrollStringVideoOverlay(BOOLEAN fEnable);

// will go and clear all displayed strings off the screen
void ClearDisplayedListOfTacticalStrings();

// clear ALL strings in the tactical Message Queue
void ClearTacticalMessageQueue();

void LoadMapScreenMessagesFromSaveGameFile(HWFILE, bool stracLinuxFormat);
void SaveMapScreenMessagesToSaveGameFile(HWFILE);

// use these if you are not Kris
void HideMessagesDuringNPCDialogue();
void UnHideMessagesDuringNPCDialogue();

// disable and enable scroll string, only to be used by Kris
void DisableScrollMessages();
void EnableScrollMessages();

extern uint8_t gubStartOfMapScreenMessageList;

#endif
