#include "SGP/ImpTGA.h"

#include <stdexcept>

#include "SGP/Buffer.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/HImage.h"
#include "SGP/LoadSaveData.h"
#include "SGP/MemMan.h"
#include "SGP/Types.h"

static SGPImage *ReadRLEColMapImage(HWFILE, uint8_t uiImgID, uint8_t uiColMap, uint16_t fContents);
static SGPImage *ReadRLERGBImage(HWFILE, uint8_t uiImgID, uint8_t uiColMap, uint16_t fContents);
static SGPImage *ReadUncompColMapImage(HWFILE, uint8_t uiImgID, uint8_t uiColMap,
                                       uint16_t fContents);
static SGPImage *ReadUncompRGBImage(HWFILE, uint8_t uiImgID, uint8_t uiColMap, uint16_t fContents);

SGPImage *LoadTGAFileToImage(char const *const filename, uint16_t const fContents) {
  uint8_t uiImgID, uiColMap, uiType;

  AutoSGPFile hFile(FileMan::openForReadingSmart(filename, true));

  FileRead(hFile, &uiImgID, sizeof(uint8_t));
  FileRead(hFile, &uiColMap, sizeof(uint8_t));
  FileRead(hFile, &uiType, sizeof(uint8_t));

  switch (uiType) {
    case 1:
      return ReadUncompColMapImage(hFile, uiImgID, uiColMap, fContents);
    case 2:
      return ReadUncompRGBImage(hFile, uiImgID, uiColMap, fContents);
    case 9:
      return ReadRLEColMapImage(hFile, uiImgID, uiColMap, fContents);
    case 10:
      return ReadRLERGBImage(hFile, uiImgID, uiColMap, fContents);
    default:
      throw std::runtime_error("Unsupported TGA format");
  }
}

static SGPImage *ReadUncompColMapImage(HWFILE const hFile, uint8_t const uiImgID,
                                       uint8_t const uiColMap, uint16_t const fContents) {
  throw std::runtime_error("TGA format 1 loading is unimplemented");
}

static SGPImage *ReadUncompRGBImage(HWFILE const f, uint8_t const uiImgID, uint8_t const uiColMap,
                                    uint16_t const contents) {
  uint16_t uiColMapLength;
  uint16_t uiWidth;
  uint16_t uiHeight;
  uint8_t uiImagePixelSize;

  uint8_t data[15];
  FileRead(f, data, sizeof(data));

  uint8_t const *d = data;
  EXTR_SKIP(d, 2)  // colour map origin
  EXTR_U16(d, uiColMapLength)
  EXTR_SKIP(d, 5)        // colour map entry size, x origin, y origin
  EXTR_U16(d, uiWidth)   // XXX unaligned
  EXTR_U16(d, uiHeight)  // XXX unaligned
  EXTR_U8(d, uiImagePixelSize)
  EXTR_SKIP(d, 1)  // image descriptor
  Assert(d == endof(data));

  // skip the id
  FileSeek(f, uiImgID, FILE_SEEK_FROM_CURRENT);

  // skip the colour map
  if (uiColMap != 0) {
    FileSeek(f, uiColMapLength * (uiImagePixelSize / 8), FILE_SEEK_FROM_CURRENT);
  }

  AutoSGPImage img(new SGPImage(uiWidth, uiHeight, uiImagePixelSize));

  if (contents & IMAGE_BITMAPDATA) {
    if (uiImagePixelSize == 16) {
      uint16_t *const img_data =
          (uint16_t *)(uint8_t *)img->pImageData.Allocate(uiWidth * uiHeight * 2);
      // Data is stored top-bottom - reverse for SGPImage format
      for (size_t y = uiHeight; y != 0;) {
        FileRead(f, &img_data[uiWidth * --y], uiWidth * 2);
      }
    } else if (uiImagePixelSize == 24) {
      uint8_t *const img_data = img->pImageData.Allocate(uiWidth * uiHeight * 3);
      for (size_t y = uiHeight; y != 0;) {
        uint8_t *const line = &img_data[uiWidth * 3 * --y];
        for (uint32_t x = 0; x < uiWidth; ++x) {
          uint8_t bgr[3];
          FileRead(f, bgr, sizeof(bgr));
          line[x * 3] = bgr[2];
          line[x * 3 + 1] = bgr[1];
          line[x * 3 + 2] = bgr[0];
        }
      }
    } else {
      throw std::runtime_error("Failed to load TGA with unsupported colour depth");
    }
    img->fFlags |= IMAGE_BITMAPDATA;
  }

  return img.Release();
}

static SGPImage *ReadRLEColMapImage(HWFILE const hFile, uint8_t const uiImgID,
                                    uint8_t const uiColMap, uint16_t const fContents) {
  throw std::runtime_error("TGA format 9 loading is unimplemented");
}

static SGPImage *ReadRLERGBImage(HWFILE const hFile, uint8_t const uiImgID, uint8_t const uiColMap,
                                 uint16_t const fContents) {
  throw std::runtime_error("TGA format 10 loading is unimplemented");
}
