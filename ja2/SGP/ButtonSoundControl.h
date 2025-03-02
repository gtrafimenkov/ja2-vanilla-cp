// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __BUTTON_SOUND_CONTROL_H
#define __BUTTON_SOUND_CONTROL_H

#include "SGP/ButtonSystem.h"

enum ButtonSoundScheme {
  BUTTON_SOUND_SCHEME_NONE,
  BUTTON_SOUND_SCHEME_GENERIC,
  BUTTON_SOUND_SCHEME_VERYSMALLSWITCH1,
  BUTTON_SOUND_SCHEME_VERYSMALLSWITCH2,
  BUTTON_SOUND_SCHEME_SMALLSWITCH1,
  BUTTON_SOUND_SCHEME_SMALLSWITCH2,
  BUTTON_SOUND_SCHEME_SMALLSWITCH3,
  BUTTON_SOUND_SCHEME_BIGSWITCH3,
  BUTTON_SOUND_SCHEME_COMPUTERBEEP2,
  BUTTON_SOUND_SCHEME_COMPUTERSWITCH1,
};

void SpecifyButtonSoundScheme(GUIButtonRef, ButtonSoundScheme);

enum ButtonSound {
  BUTTON_SOUND_NONE,
  BUTTON_SOUND_CLICKED_ON,
  BUTTON_SOUND_CLICKED_OFF,
  BUTTON_SOUND_DISABLED_CLICK
};

void PlayButtonSound(GUI_BUTTON const *, ButtonSound);

#endif
