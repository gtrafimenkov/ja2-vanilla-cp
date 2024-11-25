// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef GAP_H
#define GAP_H

#include "Tactical/Faces.h"

void AudioGapListDone(AudioGapList *pGapList);
BOOLEAN PollAudioGap(uint32_t uiSampleNum, AudioGapList *pGapList);
uint32_t PlayJA2GapSample(const char *zSoundFile, uint32_t ubVolume, uint32_t ubLoops,
                          uint32_t uiPan, AudioGapList *pData);

#endif
