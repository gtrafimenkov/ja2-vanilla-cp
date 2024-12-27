// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "LoadSaveEMail.h"

#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/LoadSaveData.h"

static void LoadEMailFromFile(HWFILE const File) {
  uint32_t uiSizeOfSubject;
  FileRead(File, &uiSizeOfSubject, sizeof(uint32_t));       // XXX HACK000B
  FileSeek(File, uiSizeOfSubject, FILE_SEEK_FROM_CURRENT);  // XXX HACK000B

  uint16_t usOffset;
  uint16_t usLength;
  uint8_t ubSender;
  uint32_t iDate;
  int32_t iFirstData;
  uint32_t uiSecondData;
  BOOLEAN fRead;

  uint8_t Data[44];
  FileRead(File, Data, sizeof(Data));

  uint8_t *S = Data;
  EXTR_U16(S, usOffset)
  EXTR_U16(S, usLength)
  EXTR_U8(S, ubSender)
  EXTR_SKIP(S, 3)
  EXTR_U32(S, iDate)
  EXTR_SKIP(S, 4)
  EXTR_I32(S, iFirstData)
  EXTR_U32(S, uiSecondData)
  EXTR_SKIP(S, 16)
  EXTR_BOOL(S, fRead)
  EXTR_SKIP(S, 3)
  Assert(S == endof(Data));

  AddEmailMessage(usOffset, usLength, iDate, ubSender, fRead, iFirstData, uiSecondData);
}

void LoadEmailFromSavedGame(HWFILE const File) {
  ShutDownEmailList();

  uint32_t uiNumOfEmails;
  FileRead(File, &uiNumOfEmails, sizeof(uint32_t));

  for (uint32_t cnt = 0; cnt < uiNumOfEmails; cnt++) {
    LoadEMailFromFile(File);
  }
}

static void SaveEMailIntoFile(HWFILE const File, Email const *const Mail) {
  uint8_t Data[48];

  uint8_t *D = Data;
  INJ_U32(D, 0)  // was size of subject
  INJ_U16(D, Mail->usOffset)
  INJ_U16(D, Mail->usLength)
  INJ_U8(D, Mail->ubSender)
  INJ_SKIP(D, 3)
  INJ_U32(D, Mail->iDate)
  INJ_SKIP(D, 4)
  INJ_I32(D, Mail->iFirstData)
  INJ_U32(D, Mail->uiSecondData)
  INJ_SKIP(D, 16)
  INJ_BOOL(D, Mail->fRead)
  INJ_SKIP(D, 3)
  Assert(D == endof(Data));

  FileWrite(File, Data, sizeof(Data));
}

void SaveEmailToSavedGame(HWFILE const File) {
  const Email *pEmail;

  // Count the emails
  uint32_t uiNumOfEmails = 0;
  for (pEmail = pEmailList; pEmail != NULL; pEmail = pEmail->Next) {
    uiNumOfEmails++;
  }
  FileWrite(File, &uiNumOfEmails, sizeof(uint32_t));

  for (pEmail = pEmailList; pEmail != NULL; pEmail = pEmail->Next) {
    SaveEMailIntoFile(File, pEmail);
  }
}
