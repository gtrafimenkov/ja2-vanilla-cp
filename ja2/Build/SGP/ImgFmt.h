#ifndef IMGFMT_H
#define IMGFMT_H

// Sir-Tech's Crazy Image (STCI) file format specifications.  Each file is
// composed of: 1		ImageFileHeader, uncompressed *		Palette
// (STCI_INDEXED, size = uiNumberOfColours * PALETTE_ELEMENT_SIZE), uncompressed
// *		SubRectInfo's (usNumberOfRects > 0, size = usNumberOfSubRects *
// sizeof(SubRectInfo) ), uncompressed
// *		Bytes of image data, possibly compressed

#include "SGP/Types.h"

#define STCI_ID_STRING "STCI"
#define STCI_ID_LEN 4

#define STCI_ETRLE_COMPRESSED 0x0020
#define STCI_ZLIB_COMPRESSED 0x0010
#define STCI_INDEXED 0x0008
#define STCI_RGB 0x0004
#define STCI_ALPHA 0x0002
#define STCI_TRANSPARENT 0x0001

// ETRLE defines
#define COMPRESS_TRANSPARENT 0x80
#define COMPRESS_NON_TRANSPARENT 0x00
#define COMPRESS_RUN_LIMIT 0x7F

// NB if you're going to change the header definition:
// - make sure that everything in this header is nicely aligned
// - don't exceed the 64-byte maximum
struct STCIHeader {
  uint8_t cID[STCI_ID_LEN];
  uint32_t uiOriginalSize;
  uint32_t uiStoredSize;  // equal to uiOriginalSize if data uncompressed
  uint32_t uiTransparentValue;
  uint32_t fFlags;
  uint16_t usHeight;
  uint16_t usWidth;
  union {
    struct {
      uint32_t uiRedMask;
      uint32_t uiGreenMask;
      uint32_t uiBlueMask;
      uint32_t uiAlphaMask;
      uint8_t ubRedDepth;
      uint8_t ubGreenDepth;
      uint8_t ubBlueDepth;
      uint8_t ubAlphaDepth;
    } RGB;
    struct {  // For indexed files, the palette will contain 3 separate bytes for
              // red, green, and blue
      uint32_t uiNumberOfColours;
      uint16_t usNumberOfSubImages;
      uint8_t ubRedDepth;
      uint8_t ubGreenDepth;
      uint8_t ubBlueDepth;
      uint8_t cIndexedUnused[11];  // XXX HACK000B
    } Indexed;
  };
  uint8_t ubDepth;  // size in bits of one pixel as stored in the file
  uint32_t uiAppDataSize;
  uint8_t cUnused[12];  // XXX HACK000B
};

struct STCISubImage {
  uint32_t uiDataOffset;
  uint32_t uiDataLength;
  int16_t sOffsetX;
  int16_t sOffsetY;
  uint16_t usHeight;
  uint16_t usWidth;
};

struct STCIPaletteElement {
  uint8_t ubRed;
  uint8_t ubGreen;
  uint8_t ubBlue;
};

#endif
