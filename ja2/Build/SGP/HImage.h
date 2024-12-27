// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __IMAGE_H
#define __IMAGE_H

#include "SGP/AutoPtr.h"
#include "SGP/Buffer.h"
#include "SGP/Types.h"

// The HIMAGE module provides a common interface for managing image data. This
// module includes:
// - A set of data structures representing image data. Data can be 8 or 16 bpp
// and/or
//   compressed
// - A set of file loaders which load specific file formats into the internal
// data format
// - A set of blitters which blt the data to memory
// - A comprehensive automatic blitter which blits the appropriate type based on
// the
//   image header.

// Defines for buffer bit depth
#define BUFFER_8BPP 0x1
#define BUFFER_16BPP 0x2

// Defines for image charactoristics
#define IMAGE_TRLECOMPRESSED 0x0002
#define IMAGE_PALETTE 0x0004
#define IMAGE_BITMAPDATA 0x0008
#define IMAGE_APPDATA 0x0010
#define IMAGE_ALLIMAGEDATA 0x000C
#define IMAGE_ALLDATA 0x001C

#define AUX_FULL_TILE 0x01
#define AUX_ANIMATED_TILE 0x02
#define AUX_DYNAMIC_TILE 0x04
#define AUX_INTERACTIVE_TILE 0x08
#define AUX_IGNORES_HEIGHT 0x10
#define AUX_USES_LAND_Z 0x20

struct AuxObjectData {
  uint8_t ubWallOrientation;
  uint8_t ubNumberOfTiles;
  uint16_t usTileLocIndex;
  uint8_t ubUnused1[3];  // XXX HACK000B
  uint8_t ubCurrentFrame;
  uint8_t ubNumberOfFrames;
  uint8_t fFlags;
  uint8_t ubUnused[6];  // XXX HACK000B
};

struct RelTileLoc {
  int8_t bTileOffsetX;
  int8_t bTileOffsetY;
};  // relative tile location

// TRLE subimage structure, mirroring that of ST(C)I
struct ETRLEObject {
  uint32_t uiDataOffset;
  uint32_t uiDataLength;
  int16_t sOffsetX;
  int16_t sOffsetY;
  uint16_t usHeight;
  uint16_t usWidth;
};

struct ETRLEData {
  void *pPixData;
  uint32_t uiSizePixData;
  ETRLEObject *pETRLEObject;
  uint16_t usNumberOfObjects;
};

// Image header structure
struct SGPImage {
  SGPImage(uint16_t const w, uint16_t const h, uint8_t const bpp)
      : usWidth(w),
        usHeight(h),
        ubBitDepth(bpp),
        fFlags(),
        uiAppDataSize(),
        uiSizePixData(),
        usNumberOfObjects() {}

  uint16_t usWidth;
  uint16_t usHeight;
  uint8_t ubBitDepth;
  uint16_t fFlags;
  SGP::Buffer<SGPPaletteEntry> pPalette;
  SGP::Buffer<uint16_t> pui16BPPPalette;
  SGP::Buffer<uint8_t> pAppData;
  uint32_t uiAppDataSize;
  SGP::Buffer<uint8_t> pImageData;
  uint32_t uiSizePixData;
  SGP::Buffer<ETRLEObject> pETRLEObject;
  uint16_t usNumberOfObjects;
};

#define SGPGetRValue(rgb) ((uint8_t)(rgb))
#define SGPGetBValue(rgb) ((uint8_t)((rgb) >> 16))
#define SGPGetGValue(rgb) ((uint8_t)(((uint16_t)(rgb)) >> 8))

SGPImage *CreateImage(const char *ImageFile, uint16_t fContents);

// This function will run the appropriate copy function based on the type of
// SGPImage object
BOOLEAN CopyImageToBuffer(SGPImage const *, uint32_t fBufferType, uint8_t *pDestBuf,
                          uint16_t usDestWidth, uint16_t usDestHeight, uint16_t usX, uint16_t usY,
                          SGPBox const *src_rect);

// This function will create a buffer in memory of ETRLE data, excluding palette
void GetETRLEImageData(SGPImage const *, ETRLEData *);

// UTILITY FUNCTIONS

// Used to create a 16BPP Palette from an 8 bit palette, found in himage.c
uint16_t *Create16BPPPaletteShaded(const SGPPaletteEntry *pPalette, uint32_t rscale,
                                   uint32_t gscale, uint32_t bscale, BOOLEAN mono);
uint16_t *Create16BPPPalette(const SGPPaletteEntry *pPalette);
uint16_t Get16BPPColor(uint32_t RGBValue);
uint32_t GetRGBColor(uint16_t Value16BPP);

extern uint16_t gusRedMask;
extern uint16_t gusGreenMask;
extern uint16_t gusBlueMask;
extern int16_t gusRedShift;
extern int16_t gusBlueShift;
extern int16_t gusGreenShift;

// used to convert 565 RGB data into different bit-formats
void ConvertRGBDistribution565To555(uint16_t *p16BPPData, uint32_t uiNumberOfPixels);
void ConvertRGBDistribution565To655(uint16_t *p16BPPData, uint32_t uiNumberOfPixels);
void ConvertRGBDistribution565To556(uint16_t *p16BPPData, uint32_t uiNumberOfPixels);
void ConvertRGBDistribution565ToAny(uint16_t *p16BPPData, uint32_t uiNumberOfPixels);

typedef SGP::AutoPtr<SGPImage> AutoSGPImage;

#endif
