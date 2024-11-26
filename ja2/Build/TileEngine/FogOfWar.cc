#include "TileEngine/FogOfWar.h"

#include "SGP/Types.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/SimpleRenderUtils.h"
#include "TileEngine/WorldDef.h"

// When line of sight reaches a gridno, and there is a light there, it turns it
// on. This is only done in the cave levels.
void RemoveFogFromGridNo(uint32_t uiGridNo) {
  int32_t x, y;
  x = uiGridNo % WORLD_COLS;
  y = uiGridNo / WORLD_COLS;
  FOR_EACH_LIGHT_SPRITE(l) {
    if (l->iX == x && l->iY == y) {
      if (!(l->uiFlags & LIGHT_SPR_ON)) {
        LightSpritePower(l, TRUE);
        LightDraw(l);
        MarkWorldDirty();
        return;
      }
    }
  }
}
