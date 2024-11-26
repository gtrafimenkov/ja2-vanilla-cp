#include "Tactical/AnimationCache.h"

#include "SGP/MemMan.h"
#include "SGP/Types.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Overhead.h"
#include "Utils/DebugControl.h"

#define EMPTY_CACHE_ENTRY 65000

static const uint32_t guiCacheSize = MIN_CACHE_SIZE;

void InitAnimationCache(uint16_t const usSoldierID, AnimationSurfaceCacheType *const pAnimCache) {
  uint32_t cnt;

  // Allocate entries
  AnimDebugMsg(String("*** Initializing anim cache surface for soldier %d", usSoldierID));
  pAnimCache->usCachedSurfaces = MALLOCN(uint16_t, guiCacheSize);

  AnimDebugMsg(String("*** Initializing anim cache hit counter for soldier %d", usSoldierID));
  pAnimCache->sCacheHits = MALLOCN(int16_t, guiCacheSize);

  // Zero entries
  for (cnt = 0; cnt < guiCacheSize; cnt++) {
    pAnimCache->usCachedSurfaces[cnt] = EMPTY_CACHE_ENTRY;
    pAnimCache->sCacheHits[cnt] = 0;
  }
  pAnimCache->ubCacheSize = 0;

  // Zero surface databse history for this soldeir
  ClearAnimationSurfacesUsageHistory(usSoldierID);
}

void DeleteAnimationCache(uint16_t usSoldierID, AnimationSurfaceCacheType *pAnimCache) {
  // Allocate entries
  if (pAnimCache->usCachedSurfaces != NULL) {
    AnimDebugMsg(String("*** Removing Anim Cache surface for soldier %d", usSoldierID));
    MemFree(pAnimCache->usCachedSurfaces);
  }

  if (pAnimCache->sCacheHits != NULL) {
    AnimDebugMsg(String("*** Removing Anim Cache hit counter for soldier %d", usSoldierID));
    MemFree(pAnimCache->sCacheHits);
  }
}

void GetCachedAnimationSurface(uint16_t const usSoldierID,
                               AnimationSurfaceCacheType *const pAnimCache,
                               uint16_t const usSurfaceIndex, uint16_t const usCurrentAnimation) {
  uint8_t cnt;
  uint8_t ubLowestIndex = 0;
  int16_t sMostHits = (int16_t)32000;
  uint16_t usCurrentAnimSurface;

  // Check to see if surface exists already
  for (cnt = 0; cnt < pAnimCache->ubCacheSize; cnt++) {
    if (pAnimCache->usCachedSurfaces[cnt] == usSurfaceIndex) {
      // Found surface, return
      AnimDebugMsg(String("Anim Cache: Hit %d ( Soldier %d )", usSurfaceIndex, usSoldierID));
      pAnimCache->sCacheHits[cnt]++;
      return;
    }
  }

  // Check if max size has been reached
  if (pAnimCache->ubCacheSize == guiCacheSize) {
    AnimDebugMsg(String("Anim Cache: Determining Bump Candidate ( Soldier %d )", usSoldierID));

    // Determine exisiting surface used by merc
    usCurrentAnimSurface =
        DetermineSoldierAnimationSurface(&GetMan(usSoldierID), usCurrentAnimation);
    // If the surface we are going to bump is our existing animation, reject it
    // as a candidate

    // If we get here, we need to remove an animation, pick the best one
    // Loop through and pick one with lowest cache hits
    for (cnt = 0; cnt < pAnimCache->ubCacheSize; cnt++) {
      AnimDebugMsg(String("Anim Cache: Slot %d Hits %d ( Soldier %d )", cnt,
                          pAnimCache->sCacheHits[cnt], usSoldierID));

      if (pAnimCache->usCachedSurfaces[cnt] == usCurrentAnimSurface) {
        AnimDebugMsg(
            String("Anim Cache: REJECTING Slot %d EXISTING ANIM "
                   "SURFACE ( Soldier %d )",
                   cnt, usSoldierID));
      } else {
        if (pAnimCache->sCacheHits[cnt] < sMostHits) {
          sMostHits = pAnimCache->sCacheHits[cnt];
          ubLowestIndex = cnt;
        }
      }
    }

    // Bump off lowest index
    AnimDebugMsg(String("Anim Cache: Bumping %d ( Soldier %d )", ubLowestIndex, usSoldierID));
    UnLoadAnimationSurface(usSoldierID, pAnimCache->usCachedSurfaces[ubLowestIndex]);

    // Decrement
    pAnimCache->sCacheHits[ubLowestIndex] = 0;
    pAnimCache->usCachedSurfaces[ubLowestIndex] = EMPTY_CACHE_ENTRY;
    pAnimCache->ubCacheSize--;
  }

  // If here, Insert at an empty slot
  // Find an empty slot
  for (cnt = 0; cnt < guiCacheSize; cnt++) {
    if (pAnimCache->usCachedSurfaces[cnt] == EMPTY_CACHE_ENTRY) {
      AnimDebugMsg(
          String("Anim Cache: Loading Surface %d ( Soldier %d )", usSurfaceIndex, usSoldierID));

      // Insert here
      LoadAnimationSurface(usSoldierID, usSurfaceIndex, usCurrentAnimation);
      pAnimCache->sCacheHits[cnt] = 0;
      pAnimCache->usCachedSurfaces[cnt] = usSurfaceIndex;
      pAnimCache->ubCacheSize++;

      break;
    }
  }
}

void UnLoadCachedAnimationSurfaces(uint16_t usSoldierID, AnimationSurfaceCacheType *pAnimCache) {
  uint8_t cnt;

  // Check to see if surface exists already
  for (cnt = 0; cnt < pAnimCache->ubCacheSize; cnt++) {
    if (pAnimCache->usCachedSurfaces[cnt] != EMPTY_CACHE_ENTRY) {
      UnLoadAnimationSurface(usSoldierID, pAnimCache->usCachedSurfaces[cnt]);
    }
  }
}
