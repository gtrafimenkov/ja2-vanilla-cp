// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef SLIDER_H
#define SLIDER_H

#include "SGP/Types.h"

struct SLIDER;

// defines for the different styles of sliders
enum { SLIDER_DEFAULT_STYLE, SLIDER_VERTICAL_STEEL, NUM_SLIDER_STYLES };

typedef void (*SLIDER_CHANGE_CALLBACK)(int32_t);

SLIDER *AddSlider(uint8_t ubStyle, uint16_t usCursor, uint16_t usPosX, uint16_t usPosY,
                  uint16_t usWidth, uint16_t usNumberOfIncrements, int8_t sPriority,
                  SLIDER_CHANGE_CALLBACK SliderChangeCallback);
void RemoveSliderBar(SLIDER *s);

void SetSliderValue(SLIDER *s, uint32_t uiNewValue);

void InitSlider();
void ShutDownSlider();

void RenderAllSliderBars();

#endif
