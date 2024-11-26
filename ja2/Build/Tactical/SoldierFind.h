#ifndef __SOLDIER_FIND_H
#define __SOLDIER_FIND_H

#include "JA2Types.h"

#define FIND_SOLDIER_GRIDNO 0x000000004
#define FIND_SOLDIER_SAMELEVEL 0x000000008
#define FIND_SOLDIER_BEGINSTACK 0x000000040

// RETURN FLAGS FOR FINDSOLDIER
enum SoldierFindFlags {
  NO_MERC = 0,
  SELECTED_MERC = 0x000000002,
  OWNED_MERC = 0x000000004,
  ENEMY_MERC = 0x000000008,
  UNCONSCIOUS_MERC = 0x000000020,
  DEAD_MERC = 0x000000040,
  VISIBLE_MERC = 0x000000080,
  NOINTERRUPT_MERC = 0x000000200,
  NEUTRAL_MERC = 0x000000400
};
ENUM_BITSET(SoldierFindFlags)

#define FINDSOLDIERSAMELEVEL(l) (FIND_SOLDIER_SAMELEVEL | (l) << 16)

SOLDIERTYPE *FindSoldierFromMouse();
SOLDIERTYPE *FindSoldier(GridNo, uint32_t flags);

bool IsOwnedMerc(SOLDIERTYPE const &);
SoldierFindFlags GetSoldierFindFlags(SOLDIERTYPE const &);

BOOLEAN CycleSoldierFindStack(uint16_t usMapPos);

bool GridNoOnScreen(GridNo);

BOOLEAN SoldierOnScreen(const SOLDIERTYPE *s);
BOOLEAN SoldierLocationRelativeToScreen(int16_t sGridNo, int8_t *pbDirection,
                                        uint32_t *puiScrollFlags);
void GetSoldierScreenPos(const SOLDIERTYPE *pSoldier, int16_t *psScreenX, int16_t *psScreenY);
void GetSoldierTRUEScreenPos(const SOLDIERTYPE *pSoldier, int16_t *psScreenX, int16_t *psScreenY);
BOOLEAN IsPointInSoldierBoundingBox(SOLDIERTYPE *pSoldier, int16_t sX, int16_t sY);
uint16_t FindRelativeSoldierPosition(const SOLDIERTYPE *pSoldier, int16_t sX, int16_t sY);

void GetGridNoScreenPos(int16_t sGridNo, uint8_t ubLevel, int16_t *psScreenX, int16_t *psScreenY);

BOOLEAN IsValidTargetMerc(const SOLDIERTYPE *s);

#endif
