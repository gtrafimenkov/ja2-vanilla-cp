// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _TileEngine_Smell_h
#define _TileEngine_Smell_h

#include "JA2Types.h"

enum BloodKind { HUMAN = 0, CREATURE_ON_FLOOR = 1, CREATURE_ON_ROOF = 2 };

#define NORMAL_HUMAN_SMELL_STRENGTH 10
#define COW_SMELL_STRENGTH 15
#define NORMAL_CREATURE_SMELL_STRENGTH 20

#define SMELL_TYPE_NUM_BITS 2
#define SMELL_TYPE(s) (s & 0x01)
#define SMELL_STRENGTH(s) ((s & 0xFC) >> SMELL_TYPE_NUM_BITS)

#define MAXBLOODQUANTITY 7
#define BLOODDIVISOR 10

void DecaySmells();
void DecayBloodAndSmells(uint32_t uiTime);
void DropSmell(SOLDIERTYPE &);
void DropBlood(SOLDIERTYPE const &, uint8_t strength);
void UpdateBloodGraphics(GridNo, int8_t level);
void RemoveBlood(GridNo, int8_t level);
void InternalDropBlood(GridNo, int8_t level, BloodKind, uint8_t strength, int8_t visible);

#endif
