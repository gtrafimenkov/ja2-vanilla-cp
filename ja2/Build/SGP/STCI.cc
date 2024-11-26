#include "SGP/STCI.h"

#include <stdexcept>

#include "SGP/Buffer.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/HImage.h"
#include "SGP/ImgFmt.h"
#include "SGP/MemMan.h"

#include "SDL_pixels.h"

static SGPImage *STCILoadIndexed(uint16_t contents, HWFILE, STCIHeader const *);
static SGPImage *STCILoadRGB(uint16_t contents, HWFILE, STCIHeader const *);

SGPImage *LoadSTCIFileToImage(char const *const filename, uint16_t const fContents) {
  AutoSGPFile f(FileMan::openForReadingSmart(filename, true));

  STCIHeader header;
  FileRead(f, &header, sizeof(header));
  if (memcmp(header.cID, STCI_ID_STRING, STCI_ID_LEN) != 0) {
    throw std::runtime_error("STCI file has invalid header");
  }

  if (header.fFlags & STCI_ZLIB_COMPRESSED) {
    throw std::runtime_error("Cannot handle zlib compressed STCI files");
  }

  // Determine from the header the data stored in the file. and run the
  // appropriate loader
  return header.fFlags & STCI_RGB ? STCILoadRGB(fContents, f, &header)
         : header.fFlags & STCI_INDEXED
             ? STCILoadIndexed(fContents, f, &header)
             :
             /* Unsupported type of data, or the right flags weren't set! */
             throw std::runtime_error("Unknown data organization in STCI file.");
}

static SGPImage *STCILoadRGB(uint16_t const contents, HWFILE const f,
                             STCIHeader const *const header) {
  if (contents & IMAGE_PALETTE &&
      (contents & IMAGE_ALLIMAGEDATA) != IMAGE_ALLIMAGEDATA) {  // RGB doesn't have a palette!
    throw std::logic_error("Invalid combination of content load flags");
  }

  AutoSGPImage img(new SGPImage(header->usWidth, header->usHeight, header->ubDepth));
  if (contents & IMAGE_BITMAPDATA) {
    // Allocate memory for the image data and read it in
    uint8_t *const img_data = img->pImageData.Allocate(header->uiStoredSize);
    FileRead(f, img_data, header->uiStoredSize);

    img->fFlags |= IMAGE_BITMAPDATA;

    if (header->ubDepth == 16) {
      // ASSUMPTION: file data is 565 R,G,B
      if (gusRedMask != (uint16_t)header->RGB.uiRedMask ||
          gusGreenMask != (uint16_t)header->RGB.uiGreenMask ||
          gusBlueMask != (uint16_t)header->RGB.uiBlueMask) {
        // colour distribution of the file is different from hardware!  We have
        // to change it!
        DebugMsg(TOPIC_HIMAGE, DBG_LEVEL_3, "Converting to current RGB distribution!");
        // Convert the image to the current hardware's specifications
        uint32_t const size = header->usWidth * header->usHeight;
        uint16_t *const data = (uint16_t *)(uint8_t *)img->pImageData;
        if (gusRedMask == 0x7C00 && gusGreenMask == 0x03E0 && gusBlueMask == 0x001F) {
          ConvertRGBDistribution565To555(data, size);
        } else if (gusRedMask == 0xFC00 && gusGreenMask == 0x03E0 && gusBlueMask == 0x001F) {
          ConvertRGBDistribution565To655(data, size);
        } else if (gusRedMask == 0xF800 && gusGreenMask == 0x07C0 && gusBlueMask == 0x003F) {
          ConvertRGBDistribution565To556(data, size);
        } else {
          // take the long route
          ConvertRGBDistribution565ToAny(data, size);
        }
      }
    }
  }
  return img.Release();
}

static SGPImage *STCILoadIndexed(uint16_t const contents, HWFILE const f,
                                 STCIHeader const *const header) {
  AutoSGPImage img(new SGPImage(header->usWidth, header->usHeight, header->ubDepth));
  if (contents & IMAGE_PALETTE) {  // Allocate memory for reading in the palette
    if (header->Indexed.uiNumberOfColours != 256) {
      throw std::runtime_error("Palettized image has bad palette size.");
    }

    SGP::Buffer<STCIPaletteElement> pSTCIPalette(256);

    // Read in the palette
    FileRead(f, pSTCIPalette, sizeof(*pSTCIPalette) * 256);

    SGPPaletteEntry *const palette = img->pPalette.Allocate(256);
    for (size_t i = 0; i < 256; i++) {
      palette[i].r = pSTCIPalette[i].ubRed;
      palette[i].g = pSTCIPalette[i].ubGreen;
      palette[i].b = pSTCIPalette[i].ubBlue;
      palette[i].a = 0;
    }

    img->fFlags |= IMAGE_PALETTE;
  } else if (contents & (IMAGE_BITMAPDATA | IMAGE_APPDATA)) {  // seek past the palette
    FileSeek(f, sizeof(STCIPaletteElement) * header->Indexed.uiNumberOfColours,
             FILE_SEEK_FROM_CURRENT);
  }

  if (contents & IMAGE_BITMAPDATA) {
    if (header->fFlags & STCI_ETRLE_COMPRESSED) {
      // load data for the subimage (object) structures
      Assert(sizeof(ETRLEObject) == sizeof(STCISubImage));

      uint16_t const n_subimages = header->Indexed.usNumberOfSubImages;
      img->usNumberOfObjects = n_subimages;

      ETRLEObject *const etrle_objects = img->pETRLEObject.Allocate(n_subimages);
      FileRead(f, etrle_objects, sizeof(*etrle_objects) * n_subimages);

      img->uiSizePixData = header->uiStoredSize;
      img->fFlags |= IMAGE_TRLECOMPRESSED;
    }

    uint8_t *const image_data = img->pImageData.Allocate(header->uiStoredSize);
    FileRead(f, image_data, header->uiStoredSize);

    img->fFlags |= IMAGE_BITMAPDATA;
  } else if (contents & IMAGE_APPDATA)  // then there's a point in seeking ahead
  {
    FileSeek(f, header->uiStoredSize, FILE_SEEK_FROM_CURRENT);
  }

  if (contents & IMAGE_APPDATA && header->uiAppDataSize > 0) {
    // load application-specific data
    uint8_t *const app_data = img->pAppData.Allocate(header->uiAppDataSize);
    FileRead(f, app_data, header->uiAppDataSize);

    img->uiAppDataSize = header->uiAppDataSize;
    img->fFlags |= IMAGE_APPDATA;
  } else {
    img->uiAppDataSize = 0;
  }

  return img.Release();
}

#include "gtest/gtest.h"

TEST(STCI, asserts) {
  EXPECT_EQ(sizeof(STCIHeader), 64);
  EXPECT_EQ(sizeof(STCISubImage), 16);
  EXPECT_EQ(sizeof(STCIPaletteElement), 3);
}
