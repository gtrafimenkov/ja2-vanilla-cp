// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/HImage.h"

#include <stdexcept>

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/ImpTGA.h"
#include "SGP/MemMan.h"
#include "SGP/PCX.h"
#include "SGP/STCI.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/WCheck.h"

#include "SDL_pixels.h"

// This is the color substituted to keep a 24bpp -> 16bpp color
// from going transparent (0x0000) -- DB

#define BLACK_SUBSTITUTE 0x0001

uint16_t gusRedMask = 0;
uint16_t gusGreenMask = 0;
uint16_t gusBlueMask = 0;
int16_t gusRedShift = 0;
int16_t gusBlueShift = 0;
int16_t gusGreenShift = 0;

SGPImage *CreateImage(const char *const filename, const uint16_t fContents) {
  // depending on extension of filename, use different image readers
  const char *const dot = strstr(filename, ".");
  if (!dot) throw std::logic_error("Tried to load image with no extension");
  const char *const ext = dot + 1;

  return strcasecmp(ext, "STI") == 0   ? LoadSTCIFileToImage(filename, fContents)
         : strcasecmp(ext, "PCX") == 0 ? LoadPCXFileToImage(filename, fContents)
         : strcasecmp(ext, "TGA") == 0
             ? LoadTGAFileToImage(filename, fContents)
             : throw std::logic_error("Tried to load image with unknown extension");
}

static BOOLEAN Copy8BPPImageTo8BPPBuffer(SGPImage const *const img, uint8_t *const pDestBuf,
                                         uint16_t const usDestWidth, uint16_t const usDestHeight,
                                         uint16_t const usX, uint16_t const usY,
                                         SGPBox const *const src_box) {
  CHECKF(usX < usDestWidth);
  CHECKF(usY < usDestHeight);
  CHECKF(src_box->w > 0);
  CHECKF(src_box->h > 0);

  // Determine memcopy coordinates
  uint32_t const uiSrcStart = src_box->y * img->usWidth + src_box->x;
  uint32_t const uiDestStart = usY * usDestWidth + usX;
  uint32_t const uiLineSize = src_box->w;
  uint32_t const uiNumLines = src_box->h;

  Assert(usDestWidth >= uiLineSize);
  Assert(usDestHeight >= uiNumLines);

  // Copy line by line
  uint8_t *dst = static_cast<uint8_t *>(pDestBuf) + uiDestStart;
  uint8_t const *src = static_cast<uint8_t const *>(img->pImageData) + uiSrcStart;
  for (uint32_t n = uiNumLines; n != 0; --n) {
    memcpy(dst, src, uiLineSize);
    dst += usDestWidth;
    src += img->usWidth;
  }

  return TRUE;
}

static BOOLEAN Copy16BPPImageTo16BPPBuffer(SGPImage const *const img, uint8_t *const pDestBuf,
                                           uint16_t const usDestWidth, uint16_t const usDestHeight,
                                           uint16_t const usX, uint16_t const usY,
                                           SGPBox const *const src_box) {
  CHECKF(usX < img->usWidth);
  CHECKF(usY < img->usHeight);
  CHECKF(src_box->w > 0);
  CHECKF(src_box->h > 0);

  // Determine memcopy coordinates
  uint32_t const uiSrcStart = src_box->y * img->usWidth + src_box->x;
  uint32_t const uiDestStart = usY * usDestWidth + usX;
  uint32_t const uiLineSize = src_box->w;
  uint32_t const uiNumLines = src_box->h;

  CHECKF(usDestWidth >= uiLineSize);
  CHECKF(usDestHeight >= uiNumLines);

  // Copy line by line
  uint16_t *dst = static_cast<uint16_t *>(static_cast<void *>(pDestBuf)) + uiDestStart;
  uint16_t const *src =
      static_cast<uint16_t const *>(static_cast<void const *>(img->pImageData)) + uiSrcStart;
  for (uint32_t n = uiNumLines; n != 0; --n) {
    memcpy(dst, src, uiLineSize * 2);
    dst += usDestWidth;
    src += img->usWidth;
  }

  return TRUE;
}

static BOOLEAN Copy8BPPImageTo16BPPBuffer(SGPImage const *const img, uint8_t *const pDestBuf,
                                          uint16_t const usDestWidth, uint16_t const usDestHeight,
                                          uint16_t const usX, uint16_t const usY,
                                          SGPBox const *const src_box) {
  CHECKF(img->pImageData);
  CHECKF(usX < usDestWidth);
  CHECKF(usY < usDestHeight);
  CHECKF(src_box->w > 0);
  CHECKF(src_box->h > 0);

  // Determine memcopy coordinates
  uint32_t const uiSrcStart = src_box->y * img->usWidth + src_box->x;
  uint32_t const uiDestStart = usY * usDestWidth + usX;
  uint32_t const uiLineSize = src_box->w;
  uint32_t const uiNumLines = src_box->h;

  CHECKF(usDestWidth >= uiLineSize);
  CHECKF(usDestHeight >= uiNumLines);

  // Convert to Pixel specification
  uint16_t *dst = static_cast<uint16_t *>(static_cast<void *>(pDestBuf)) + uiDestStart;
  uint8_t const *src = static_cast<uint8_t const *>(img->pImageData) + uiSrcStart;
  uint16_t const *const pal = img->pui16BPPPalette;
  for (uint32_t rows = uiNumLines; rows != 0; --rows) {
    uint16_t *dst_tmp = dst;
    uint8_t const *src_tmp = src;
    for (uint32_t cols = uiLineSize; cols != 0; --cols) {
      *dst_tmp++ = pal[*src_tmp++];
    }
    dst += usDestWidth;
    src += img->usWidth;
  }

  return TRUE;
}

BOOLEAN CopyImageToBuffer(SGPImage const *const img, uint32_t const fBufferType,
                          uint8_t *const pDestBuf, uint16_t const usDestWidth,
                          uint16_t const usDestHeight, uint16_t const usX, uint16_t const usY,
                          SGPBox const *const src_box) {
  // Use blitter based on type of image
  if (img->ubBitDepth == 8 && fBufferType == BUFFER_8BPP) {
    // Default do here
    DebugMsg(TOPIC_HIMAGE, DBG_LEVEL_2, "Copying 8 BPP Imagery.");
    return Copy8BPPImageTo8BPPBuffer(img, pDestBuf, usDestWidth, usDestHeight, usX, usY, src_box);
  } else if (img->ubBitDepth == 8 && fBufferType == BUFFER_16BPP) {
    DebugMsg(TOPIC_HIMAGE, DBG_LEVEL_3, "Copying 8 BPP Imagery to 16BPP Buffer.");
    return Copy8BPPImageTo16BPPBuffer(img, pDestBuf, usDestWidth, usDestHeight, usX, usY, src_box);
  } else if (img->ubBitDepth == 16 && fBufferType == BUFFER_16BPP) {
    DebugMsg(TOPIC_HIMAGE, DBG_LEVEL_3, "Automatically Copying 16 BPP Imagery.");
    return Copy16BPPImageTo16BPPBuffer(img, pDestBuf, usDestWidth, usDestHeight, usX, usY, src_box);
  }

  return FALSE;
}

uint16_t *Create16BPPPalette(const SGPPaletteEntry *pPalette) {
  Assert(pPalette != NULL);

  uint16_t *const p16BPPPalette = MALLOCN(uint16_t, 256);

  for (uint32_t cnt = 0; cnt < 256; cnt++) {
    uint8_t const r = pPalette[cnt].r;
    uint8_t const g = pPalette[cnt].g;
    uint8_t const b = pPalette[cnt].b;
    p16BPPPalette[cnt] = Get16BPPColor(FROMRGB(r, g, b));
  }

  return p16BPPPalette;
}

/**********************************************************************************************
 Create16BPPPaletteShaded

        Creates an 8 bit to 16 bit palette table, and modifies the colors as it
builds.

        Parameters:
                rscale, gscale, bscale:
                                Color mode: Percentages (255=100%) of color to
translate into destination palette. Mono mode:  Color for monochrome palette.
                mono:
                                TRUE or FALSE to create a monochrome palette. In
mono mode, Luminance values for colors are calculated, and the RGB color is
shaded according to each pixel's brightness.

        This can be used in several ways:

        1) To "brighten" a palette, pass down RGB values that are higher than
100% ( > 255) for all three. mono=FALSE. 2) To "darken" a palette, do the same
with less than 100% ( < 255) values. mono=FALSE.

        3) To create a "glow" palette, select mono=TRUE, and pass the color in
the RGB parameters.

        4) For gamma correction, pass in weighted values for each color.

**********************************************************************************************/
uint16_t *Create16BPPPaletteShaded(const SGPPaletteEntry *pPalette, uint32_t rscale,
                                   uint32_t gscale, uint32_t bscale, BOOLEAN mono) {
  Assert(pPalette != NULL);

  uint16_t *const p16BPPPalette = MALLOCN(uint16_t, 256);

  for (uint32_t cnt = 0; cnt < 256; cnt++) {
    uint32_t rmod;
    uint32_t gmod;
    uint32_t bmod;
    if (mono) {
      uint32_t lumin =
          (pPalette[cnt].r * 299 + pPalette[cnt].g * 587 + pPalette[cnt].b * 114) / 1000;
      rmod = rscale * lumin / 256;
      gmod = gscale * lumin / 256;
      bmod = bscale * lumin / 256;
    } else {
      rmod = rscale * pPalette[cnt].r / 256;
      gmod = gscale * pPalette[cnt].g / 256;
      bmod = bscale * pPalette[cnt].b / 256;
    }

    uint8_t r = std::min((uint8_t)rmod, (uint8_t)255);
    uint8_t g = std::min((uint8_t)gmod, (uint8_t)255);
    uint8_t b = std::min((uint8_t)bmod, (uint8_t)255);
    p16BPPPalette[cnt] = Get16BPPColor(FROMRGB(r, g, b));
  }
  return p16BPPPalette;
}

// Convert from RGB to 16 bit value
uint16_t Get16BPPColor(uint32_t RGBValue) {
  uint8_t r = SGPGetRValue(RGBValue);
  uint8_t g = SGPGetGValue(RGBValue);
  uint8_t b = SGPGetBValue(RGBValue);

  uint16_t r16 = (gusRedShift < 0 ? r >> -gusRedShift : r << gusRedShift);
  uint16_t g16 = (gusGreenShift < 0 ? g >> -gusGreenShift : g << gusGreenShift);
  uint16_t b16 = (gusBlueShift < 0 ? b >> -gusBlueShift : b << gusBlueShift);

  uint16_t usColor = (r16 & gusRedMask) | (g16 & gusGreenMask) | (b16 & gusBlueMask);

  // if our color worked out to absolute black, and the original wasn't
  // absolute black, convert it to a VERY dark grey to avoid transparency
  // problems
  if (usColor == 0 && RGBValue != 0) usColor = BLACK_SUBSTITUTE;

  return usColor;
}

// Convert from 16 BPP to RGBvalue
uint32_t GetRGBColor(uint16_t Value16BPP) {
  uint32_t r16 = Value16BPP & gusRedMask;
  uint32_t g16 = Value16BPP & gusGreenMask;
  uint32_t b16 = Value16BPP & gusBlueMask;

  uint32_t r = (gusRedShift < 0 ? r16 << -gusRedShift : r16 >> gusRedShift);
  uint32_t g = (gusGreenShift < 0 ? g16 << -gusGreenShift : g16 >> gusGreenShift);
  uint32_t b = (gusBlueShift < 0 ? b16 << -gusBlueShift : b16 >> gusBlueShift);

  r &= 0x000000ff;
  g &= 0x000000ff;
  b &= 0x000000ff;

  uint32_t val = FROMRGB(r, g, b);
  return val;
}

void GetETRLEImageData(SGPImage const *const img, ETRLEData *const buf) {
  Assert(img);
  Assert(buf);

  SGP::Buffer<ETRLEObject> etrle_objs(img->usNumberOfObjects);
  memcpy(etrle_objs, img->pETRLEObject, sizeof(*etrle_objs) * img->usNumberOfObjects);

  SGP::Buffer<uint8_t> pix_data(img->uiSizePixData);
  memcpy(pix_data, img->pImageData, sizeof(*pix_data) * img->uiSizePixData);

  buf->pPixData = pix_data.Release();
  buf->uiSizePixData = img->uiSizePixData;
  buf->pETRLEObject = etrle_objs.Release();
  buf->usNumberOfObjects = img->usNumberOfObjects;
}

void ConvertRGBDistribution565To555(uint16_t *p16BPPData, uint32_t uiNumberOfPixels) {
  for (uint16_t *Px = p16BPPData; Px != p16BPPData + uiNumberOfPixels; ++Px) {
    *Px = ((*Px >> 1) & ~0x001F) | (*Px & 0x001F);
  }
}

void ConvertRGBDistribution565To655(uint16_t *p16BPPData, uint32_t uiNumberOfPixels) {
  for (uint16_t *Px = p16BPPData; Px != p16BPPData + uiNumberOfPixels; ++Px) {
    *Px = ((*Px >> 1) & 0x03E0) | (*Px & ~0x07E0);
  }
}

void ConvertRGBDistribution565To556(uint16_t *p16BPPData, uint32_t uiNumberOfPixels) {
  for (uint16_t *Px = p16BPPData; Px != p16BPPData + uiNumberOfPixels; ++Px) {
    *Px = (*Px & ~0x003F) | ((*Px << 1) & 0x003F);
  }
}

void ConvertRGBDistribution565ToAny(uint16_t *const p16BPPData, uint32_t const uiNumberOfPixels) {
  uint16_t *px = p16BPPData;
  for (size_t n = uiNumberOfPixels; n != 0; --n) {
    // put the 565 RGB 16-bit value into a 32-bit RGB value
    uint32_t const r = (*px) >> 11;
    uint32_t const g = (*px & 0x07E0) >> 5;
    uint32_t const b = (*px & 0x001F);
    uint32_t const rgb = FROMRGB(r, g, b);
    // then convert the 32-bit RGB value to whatever 16 bit format is used
    *px++ = Get16BPPColor(rgb);
  }
}

#undef FAIL
#include "gtest/gtest.h"

TEST(HImage, asserts) {
  EXPECT_EQ(sizeof(AuxObjectData), 16);
  EXPECT_EQ(sizeof(RelTileLoc), 2);
  EXPECT_EQ(sizeof(ETRLEObject), 16);
  EXPECT_EQ(sizeof(SGPPaletteEntry), 4);
}
