// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/Smell.h"

#include <algorithm>

#include "GameSettings.h"
#include "Macro.h"
#include "SGP/Random.h"
#include "Strategic/GameClock.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SaveLoadMap.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"

/*
 * Smell & Blood system
 *
 * Smell and blood trails decay as time passes.
 *
 *             Decay Rate        Maximum Strength    Decay Time: Min Max (for
 * biggest volume)
 *
 * Smell       1 per turn              31                         31  31
 * Blood    1 every 1-3 turns           7                          7  21
 *
 * Smell has a much finer resolution so that creatures which track by smell
 * can do so effectively.
 */

/*
 * Time for some crazy-ass macros!
 * The smell byte is spit as follows:
 * O \
 * O  \
 * O   \ Smell
 * O   / Strength (only on ground)
 * O  /
 * O /
 * O >   Type of blood on roof
 * O >   Type of smell/blood on ground
 *
 * The blood byte is split as follows:
 * O \
 * O  > Blood quantity on roof
 * O /
 * O \
 * O  > Blood quantity on ground
 * O /
 * O \  Blood decay
 * O /  time (roof and ground decay together)
 */

/*
 * In these defines,
 * s indicates the smell byte, b indicates the blood byte
 */

#define SMELL_STRENGTH_MAX 63
#define BLOOD_STRENGTH_MAX 7
#define BLOOD_DELAY_MAX 3

#define SMELL_TYPE_BITS(s) (s & 0x03)

#define BLOOD_ROOF_TYPE(s) (s & 0x02)
#define BLOOD_FLOOR_TYPE(s) (s & 0x01)

#define BLOOD_ROOF_STRENGTH(b) (b & 0xE0)
#define BLOOD_FLOOR_STRENGTH(b) ((b & 0x1C) >> 2)
#define BLOOD_DELAY_TIME(b) (b & 0x03)
#define NO_BLOOD_STRENGTH(b) ((b & 0xFC) == 0)

#define DECAY_SMELL_STRENGTH(s)                     \
  {                                                 \
    uint8_t ubStrength = SMELL_STRENGTH((s));       \
    ubStrength--;                                   \
    ubStrength = ubStrength << SMELL_TYPE_NUM_BITS; \
    (s) = SMELL_TYPE_BITS((s)) | ubStrength;        \
  }

// s = smell byte
// ns = new strength
// ntf = new type on floor
// Note that the first part of the macro is designed to
// preserve the type value for the blood on the roof
#define SET_SMELL(s, ns, ntf) \
  { (s) = (BLOOD_ROOF_TYPE(s)) | SMELL_TYPE(ntf) | (ns << SMELL_TYPE_NUM_BITS); }

#define DECAY_BLOOD_DELAY_TIME(b) \
  { (b)--; }

#define SET_BLOOD_FLOOR_STRENGTH(b, nb) \
  { (b) = ((nb) << 2) | ((b) & 0xE3); }

#define SET_BLOOD_ROOF_STRENGTH(b, nb) \
  { (b) = BLOOD_FLOOR_STRENGTH((nb)) << 5 | ((b) & 0x1F); }

#define DECAY_BLOOD_FLOOR_STRENGTH(b)             \
  {                                               \
    uint8_t ubFloorStrength;                      \
    ubFloorStrength = BLOOD_FLOOR_STRENGTH((b));  \
    ubFloorStrength--;                            \
    SET_BLOOD_FLOOR_STRENGTH(b, ubFloorStrength); \
  }

#define DECAY_BLOOD_ROOF_STRENGTH(b)             \
  {                                              \
    uint8_t ubRoofStrength;                      \
    ubRoofStrength = BLOOD_ROOF_STRENGTH((b));   \
    ubRoofStrength--;                            \
    SET_BLOOD_FLOOR_STRENGTH(b, ubRoofStrength); \
  }

#define SET_BLOOD_DELAY_TIME(b) \
  { (b) = BLOOD_DELAY_TIME((uint8_t)Random(BLOOD_DELAY_MAX) + 1) | (b & 0xFC); }

#define SET_BLOOD_FLOOR_TYPE(s, ntg) \
  { (s) = BLOOD_FLOOR_TYPE(ntg) | (s & 0xFE); }

#define SET_BLOOD_ROOF_TYPE(s, ntr) \
  { (s) = BLOOD_ROOF_TYPE(ntr) | (s & 0xFD); }

void RemoveBlood(GridNo const gridno, int8_t const level) {
  MAP_ELEMENT &me = gpWorldLevelData[gridno];
  me.ubBloodInfo = 0;
  me.uiFlags |= MAPELEMENT_REEVALUATEBLOOD;
  UpdateBloodGraphics(gridno, level);
}

void DecaySmells() {
  FOR_EACH_WORLD_TILE(i) {
    uint8_t &smell = i->ubSmellInfo;
    if (smell == 0) continue;
    DECAY_SMELL_STRENGTH(smell);
    // If the strength left is 0, wipe the whole byte to clear the type
    if (SMELL_STRENGTH(smell) == 0) smell = 0;
  }
}

static void DecayBlood() {
  FOR_EACH_WORLD_TILE(pMapElement) {
    if (pMapElement->ubBloodInfo) {
      // delay blood timer!
      DECAY_BLOOD_DELAY_TIME(pMapElement->ubBloodInfo);
      if (BLOOD_DELAY_TIME(pMapElement->ubBloodInfo) == 0) {
        // Set re-evaluate flag
        pMapElement->uiFlags |= MAPELEMENT_REEVALUATEBLOOD;

        // reduce the floor blood strength if it is above zero
        if (BLOOD_FLOOR_STRENGTH(pMapElement->ubBloodInfo) > 0) {
          DECAY_BLOOD_FLOOR_STRENGTH(pMapElement->ubBloodInfo)
          if (BLOOD_FLOOR_STRENGTH(pMapElement->ubBloodInfo) == 0) {
            // delete the blood graphic on the floor!
            // then
            if (NO_BLOOD_STRENGTH(pMapElement->ubBloodInfo)) {
              // wipe the whole byte to zero
              pMapElement->ubBloodInfo = 0;
            }
          }
        }
        // reduce the roof blood strength if it is above zero
        if (BLOOD_ROOF_STRENGTH(pMapElement->ubBloodInfo) > 0) {
          DECAY_BLOOD_ROOF_STRENGTH(pMapElement->ubBloodInfo)
          if (BLOOD_ROOF_STRENGTH(pMapElement->ubBloodInfo) == 0) {
            // delete the blood graphic on the roof!
            if (NO_BLOOD_STRENGTH(pMapElement->ubBloodInfo)) {
              // wipe the whole byte to zero
              pMapElement->ubBloodInfo = 0;
            }
          }
        }

        // if any blood remaining, reset time
        if (pMapElement->ubBloodInfo) {
          SET_BLOOD_DELAY_TIME(pMapElement->ubBloodInfo);
        }
      }
      // end of blood handling
    }

    // now go on to the next gridno
  }
}

void DecayBloodAndSmells(uint32_t uiTime) {
  uint32_t uiCheckTime;

  if (!gfWorldLoaded) {
    return;
  }

  // period between checks, in game seconds
  switch (giTimeCompressMode) {
    // in time compression, let this happen every 5 REAL seconds
    case TIME_COMPRESS_5MINS:  // rate of 300 seconds per real second
      uiCheckTime = 5 * 300;
      break;
    case TIME_COMPRESS_30MINS:  // rate of 1800 seconds per real second
      uiCheckTime = 5 * 1800;
      break;
    case TIME_COMPRESS_60MINS:  // rate of 3600 seconds per real second
    case TIME_SUPER_COMPRESS:   // should not be used but just in frigging case...
      uiCheckTime = 5 * 3600;
      break;
    default:  // not compressing
      uiCheckTime = 100;
      break;
  }

  // ok so "uiDecayBloodLastUpdate" is a bit of a misnomer now
  if ((uiTime - gTacticalStatus.uiDecayBloodLastUpdate) > uiCheckTime) {
    gTacticalStatus.uiDecayBloodLastUpdate = uiTime;
    DecayBlood();
    DecaySmells();
  }
}

void DropSmell(SOLDIERTYPE &s) {
  MAP_ELEMENT *pMapElement;
  uint8_t ubOldSmell;
  uint8_t ubOldStrength;
  uint8_t ubSmell;
  uint8_t ubStrength;

  /*
   *  Here we are creating a new smell on the ground.  If there is blood in
   *  the tile, it overrides dropping smells of any type
   */

  if (s.bLevel == 0) {
    pMapElement = &(gpWorldLevelData[s.sGridNo]);
    if (pMapElement->ubBloodInfo) {
      // blood here, don't drop any smell
      return;
    }

    if (s.bNormalSmell > s.bMonsterSmell) {
      ubStrength = s.bNormalSmell - s.bMonsterSmell;
      ubSmell = HUMAN;
    } else {
      ubStrength = s.bMonsterSmell - s.bNormalSmell;
      if (ubStrength == 0) {
        // don't drop any smell
        return;
      }
      ubSmell = CREATURE_ON_FLOOR;
    }

    if (pMapElement->ubSmellInfo) {
      // smell already exists here; check to see if it's the same or not

      ubOldSmell = SMELL_TYPE(pMapElement->ubSmellInfo);
      ubOldStrength = SMELL_STRENGTH(pMapElement->ubSmellInfo);
      if (ubOldSmell == ubSmell) {
        // same smell; increase the strength to the bigger of the two strengths,
        // plus 1/5 of the smaller
        ubStrength = std::max(ubStrength, ubOldStrength) + std::min(ubStrength, ubOldStrength) / 5;
        ubStrength = std::max(ubStrength, (uint8_t)SMELL_STRENGTH_MAX);
      } else {
        // different smell; we muddy the smell by reducing the smell strength
        if (ubOldStrength > ubStrength) {
          ubOldStrength -= ubStrength / 3;
          SET_SMELL(pMapElement->ubSmellInfo, ubOldStrength, ubOldSmell);
        } else {
          ubStrength -= ubOldStrength / 3;
          if (ubStrength > 0) {
            SET_SMELL(pMapElement->ubSmellInfo, ubStrength, ubSmell);
          } else {
            // smell reduced to 0 - wipe all info on it!
            pMapElement->ubSmellInfo = 0;
          }
        }
      }
    } else {
      // the simple case, dropping a smell in a location where there is none
      SET_SMELL(pMapElement->ubSmellInfo, ubStrength, ubSmell);
    }
  }
  // otherwise skip dropping smell
}

void InternalDropBlood(GridNo const gridno, int8_t const level, BloodKind const blood_kind,
                       uint8_t strength, int8_t const visible) {
  /* Dropping some blood;
   * We can check the type of blood by consulting the type in the smell byte */

  // ATE: Send warning if dropping blood nowhere
  if (gridno == NOWHERE) {
    return;
  }

  if (Water(gridno)) return;

  // Ensure max strength is okay
  strength = std::min(strength, (uint8_t)BLOOD_STRENGTH_MAX);

  uint8_t new_strength = 0;
  MAP_ELEMENT &me = gpWorldLevelData[gridno];
  if (level == 0) {  // Dropping blood on ground
    uint8_t const old_strength = BLOOD_FLOOR_STRENGTH(me.ubBloodInfo);
    if (old_strength > 0) {
      // blood already there... we'll leave the decay time as it is
      if (BLOOD_FLOOR_TYPE(me.ubBloodInfo) == blood_kind) {  // Combine blood strengths
        new_strength = std::min(old_strength + strength, BLOOD_STRENGTH_MAX);
        SET_BLOOD_FLOOR_STRENGTH(me.ubBloodInfo, new_strength);
      } else {                          // Replace the existing blood if more is being dropped than
                                        // exists
        if (strength > old_strength) {  // Replace
          SET_BLOOD_FLOOR_STRENGTH(me.ubBloodInfo, strength);
        }
        // Else we don't drop anything at all
      }
    } else {
      // No blood on the ground yet, so drop this amount.
      // Set decay time
      SET_BLOOD_DELAY_TIME(me.ubBloodInfo);
      SET_BLOOD_FLOOR_STRENGTH(me.ubBloodInfo, strength);
      // NB blood floor type stored in smell byte!
      SET_BLOOD_FLOOR_TYPE(me.ubSmellInfo, blood_kind);
    }
  } else {  // Dropping blood on roof
    uint8_t const old_strength = BLOOD_ROOF_STRENGTH(me.ubBloodInfo);
    if (old_strength > 0) {
      // Blood already there, we'll leave the decay time as it is
      if (BLOOD_ROOF_TYPE(me.ubSmellInfo) == blood_kind) {  // Combine blood strengths
        new_strength = std::max(old_strength, strength) + 1;
        // make sure the strength is legal
        new_strength = std::max(new_strength, (uint8_t)BLOOD_STRENGTH_MAX);
        SET_BLOOD_ROOF_STRENGTH(me.ubBloodInfo, new_strength);
      } else {                          // Replace the existing blood if more is being dropped than
                                        // exists
        if (strength > old_strength) {  // Replace
          SET_BLOOD_ROOF_STRENGTH(me.ubBloodInfo, strength);
        }
        // Else we don't drop anything at all
      }
    } else {
      // No blood on the roof yet, so drop this amount.
      // Set decay time
      SET_BLOOD_DELAY_TIME(me.ubBloodInfo);
      SET_BLOOD_ROOF_STRENGTH(me.ubBloodInfo,
                              new_strength);  // XXX new_strength always 0 here
      SET_BLOOD_ROOF_TYPE(me.ubSmellInfo, blood_kind);
    }
  }

  me.uiFlags |= MAPELEMENT_REEVALUATEBLOOD;

  if (visible != -1) UpdateBloodGraphics(gridno, level);
}

void DropBlood(SOLDIERTYPE const &s, uint8_t const strength) {
  // Figure out the kind of blood that we're dropping
  BloodKind const b = !(s.uiStatusFlags & SOLDIER_MONSTER) ? HUMAN
                      : s.bLevel == 0                      ? CREATURE_ON_FLOOR
                                                           : CREATURE_ON_ROOF;
  InternalDropBlood(s.sGridNo, s.bLevel, b, strength, s.bVisible);
}

void UpdateBloodGraphics(GridNo const gridno, int8_t const level) {
  // Based on level, type, display graphics for blood

  // Check for blood option
  if (!gGameSettings.fOptions[TOPTION_BLOOD_N_GORE]) return;

  MAP_ELEMENT &me = gpWorldLevelData[gridno];
  if (!(me.uiFlags & MAPELEMENT_REEVALUATEBLOOD)) return;
  me.uiFlags &= ~MAPELEMENT_REEVALUATEBLOOD;

  if (level == 0) {  // Ground
    // Remove tile graphic if one exists.
    if (LEVELNODE const *const n =
            TypeRangeExistsInObjectLayer(gridno, HUMANBLOOD, CREATUREBLOOD)) {
      RemoveObject(gridno, n->usIndex);
    }

    // Pick new one. based on strength and randomness
    int8_t const strength = BLOOD_FLOOR_STRENGTH(me.ubBloodInfo);
    if (strength == 0) return;

    uint16_t const index = Random(4) * 4 + 3 - strength / 2U;
    uint32_t const type = BLOOD_FLOOR_TYPE(me.ubSmellInfo) == HUMAN ? HUMANBLOOD : CREATUREBLOOD;
    uint16_t const new_index = GetTileIndexFromTypeSubIndex(type, index + 1);
    AddObjectToHead(gridno, new_index);

    // Update rendering
    me.uiFlags |= MAPELEMENT_REDRAW;
    SetRenderFlags(RENDER_FLAG_MARKED);
  } else {  // Roof
            // XXX no visible blood on roofs
  }
}
