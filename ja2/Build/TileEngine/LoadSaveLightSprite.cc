#include "TileEngine/LoadSaveLightSprite.h"

#include "JAScreens.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/LoadSaveData.h"
#include "SGP/MemMan.h"
#include "ScreenIDs.h"
#include "SysGlobals.h"
#include "TileEngine/Environment.h"

void ExtractLightSprite(HWFILE const f, uint32_t const light_time) {
  int16_t x;
  int16_t y;
  uint32_t flags;
  uint8_t str_len;

  uint8_t data[25];
  FileRead(f, data, sizeof(data));

  uint8_t const *d = data;
  EXTR_I16(d, x)
  EXTR_I16(d, y)
  EXTR_SKIP(d, 12)
  EXTR_U32(d, flags)
  EXTR_SKIP(d, 4)
  EXTR_U8(d, str_len)
  Assert(d == endof(data));

  char *template_name = MALLOCN(char, str_len);
  FileRead(f, template_name, str_len);
  template_name[str_len - 1] = '\0';

  LIGHT_SPRITE *const l = LightSpriteCreate(template_name);
  // if this fails, then we will ignore the light.
  // ATE: Don't add ANY lights of mapscreen util is on
  if (l != NULL && guiCurrentScreen != MAPUTILITY_SCREEN) {
    // power only valid lights
    if (gfEditMode ||
        (!gfCaves && (flags & light_time || !(flags & (LIGHT_PRIMETIME | LIGHT_NIGHTTIME))))) {
      LightSpritePower(l, TRUE);
    }
    LightSpritePosition(l, x, y);
    if (flags & LIGHT_PRIMETIME) {
      l->uiFlags |= LIGHT_PRIMETIME;
    } else if (flags & LIGHT_NIGHTTIME) {
      l->uiFlags |= LIGHT_NIGHTTIME;
    }
  }
  MemFree(template_name);
}

void InjectLightSpriteIntoFile(HWFILE const file, LIGHT_SPRITE const *const l) {
  uint8_t data[24];

  uint8_t *d = data;
  INJ_I16(d, l->iX)
  INJ_I16(d, l->iY)
  INJ_SKIP(d, 12)
  INJ_U32(d, l->uiFlags)
  INJ_SKIP(d, 4)
  Assert(d == endof(data));

  FileWrite(file, data, sizeof(data));

  const char *const light_name = LightSpriteGetTypeName(l);
  const uint8_t str_len = strlen(light_name) + 1;
  FileWrite(file, &str_len, sizeof(str_len));
  FileWrite(file, light_name, str_len);
}
