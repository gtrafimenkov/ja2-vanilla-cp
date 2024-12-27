// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef MERCTEXTBOX_H
#define MERCTEXTBOX_H

#include "JA2Types.h"
#include "SGP/AutoObj.h"

enum MercPopupBoxFlags {
  MERC_POPUP_PREPARE_FLAGS_NONE = 0,
  MERC_POPUP_PREPARE_FLAGS_TRANS_BACK = 1U << 0,
  MERC_POPUP_PREPARE_FLAGS_STOPICON = 1U << 1,
  MERC_POPUP_PREPARE_FLAGS_SKULLICON = 1U << 2
};
ENUM_BITSET(MercPopupBoxFlags)

void InitMercPopupBox();

enum MercPopUpBackground {
  BASIC_MERC_POPUP_BACKGROUND = 0,
  WHITE_MERC_POPUP_BACKGROUND,
  GREY_MERC_POPUP_BACKGROUND,
  DIALOG_MERC_POPUP_BACKGROUND,
  LAPTOP_POPUP_BACKGROUND,
  IMP_POPUP_BACKGROUND
};

enum MercPopUpBorder {
  BASIC_MERC_POPUP_BORDER = 0,
  RED_MERC_POPUP_BORDER,
  BLUE_MERC_POPUP_BORDER,
  DIALOG_MERC_POPUP_BORDER,
  LAPTOP_POP_BORDER
};

// create a pop up box if needed, return null pointer on failure
MercPopUpBox *PrepareMercPopupBox(MercPopUpBox *, MercPopUpBackground, MercPopUpBorder,
                                  wchar_t const *pString, uint16_t usWidth, uint16_t usMarginX,
                                  uint16_t usMarginTopY, uint16_t usMarginBottomY,
                                  uint16_t *pActualWidth, uint16_t *pActualHeight,
                                  MercPopupBoxFlags flags = MERC_POPUP_PREPARE_FLAGS_NONE);

void RemoveMercPopupBox(MercPopUpBox *);

void RenderMercPopUpBox(MercPopUpBox const *, int16_t sDestX, int16_t sDestY, SGPVSurface *buffer);

typedef SGP::AutoObj<MercPopUpBox, RemoveMercPopupBox> AutoMercPopUpBox;

#endif
