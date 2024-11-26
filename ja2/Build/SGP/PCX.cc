#include "SGP/PCX.h"

#include <stdexcept>

#include "SGP/Buffer.h"
#include "SGP/FileMan.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"

#include "SDL_pixels.h"

struct PcxHeader {
  uint8_t ubManufacturer;
  uint8_t ubVersion;
  uint8_t ubEncoding;
  uint8_t ubBitsPerPixel;
  uint16_t usLeft;
  uint16_t usTop;
  uint16_t usRight;
  uint16_t usBottom;
  uint16_t usHorRez;
  uint16_t usVerRez;
  uint8_t ubEgaPalette[48];
  uint8_t ubReserved;
  uint8_t ubColorPlanes;
  uint16_t usBytesPerLine;
  uint16_t usPaletteType;
  uint8_t ubFiller[58];
};

static void BlitPcxToBuffer(uint8_t const *src, uint8_t *dst, uint16_t w, uint16_t h);

SGPImage *LoadPCXFileToImage(char const *const filename, uint16_t const contents) {
  AutoSGPFile f(FileMan::openForReadingSmart(filename, true));

  PcxHeader header;
  FileRead(f, &header, sizeof(header));
  if (header.ubManufacturer != 10 || header.ubEncoding != 1) {
    throw std::runtime_error("PCX file has invalid header");
  }

  uint32_t const file_size = FileGetSize(f);
  uint32_t const buffer_size = file_size - sizeof(PcxHeader) - 768;

  SGP::Buffer<uint8_t> pcx_buffer(buffer_size);
  FileRead(f, pcx_buffer, buffer_size);

  uint8_t palette[768];
  FileRead(f, palette, sizeof(palette));

  uint16_t const w = header.usRight - header.usLeft + 1;
  uint16_t const h = header.usBottom - header.usTop + 1;

  AutoSGPImage img(new SGPImage(w, h, 8));
  // Set some header information
  img->fFlags |= contents;

  // Read and allocate bitmap block if requested
  if (contents & IMAGE_BITMAPDATA) {
    uint8_t *const img_data = img->pImageData.Allocate(w * h);
    BlitPcxToBuffer(pcx_buffer, img_data, w, h);
  }

  if (contents & IMAGE_PALETTE) {
    SGPPaletteEntry *const dst = img->pPalette.Allocate(256);
    for (size_t i = 0; i < 256; ++i) {
      dst[i].r = palette[i * 3 + 0];
      dst[i].g = palette[i * 3 + 1];
      dst[i].b = palette[i * 3 + 2];
      dst[i].a = 0;
    }
    img->pui16BPPPalette = Create16BPPPalette(dst);
  }

  return img.Release();
}

static void BlitPcxToBuffer(uint8_t const *src, uint8_t *dst, uint16_t const w, uint16_t const h) {
  for (size_t n = w * h; n != 0;) {
    if (*src >= 0xC0) {
      size_t n_px = *src++ & 0x3F;
      uint8_t const colour = *src++;
      if (n_px > n) n_px = n;
      n -= n_px;
      for (; n_px != 0; --n_px) *dst++ = colour;
    } else {
      --n;
      *dst++ = *src++;
    }
  }
}

#include "gtest/gtest.h"

TEST(PCX, asserts) { EXPECT_EQ(sizeof(PcxHeader), 128); }
