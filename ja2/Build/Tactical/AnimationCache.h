#ifndef __ANIMATION_CACHE_H
#define __ANIMATION_CACHE_H

#include "SGP/Types.h"

#define MAX_CACHE_SIZE 20
#define MIN_CACHE_SIZE 2

struct AnimationSurfaceCacheType {
  uint16_t *usCachedSurfaces;
  int16_t *sCacheHits;
  uint8_t ubCacheSize;
};

void GetCachedAnimationSurface(uint16_t usSoldierID, AnimationSurfaceCacheType *pAnimCache,
                               uint16_t usSurfaceIndex, uint16_t usCurrentAnimation);
void InitAnimationCache(uint16_t usSoldierID, AnimationSurfaceCacheType *);
void DeleteAnimationCache(uint16_t usSoldierID, AnimationSurfaceCacheType *pAnimCache);
void UnLoadCachedAnimationSurfaces(uint16_t usSoldierID, AnimationSurfaceCacheType *pAnimCache);

#endif
