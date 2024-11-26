#include "SGP/VObject.h"

#include <stdexcept>
#include <string.h>

#include "SGP/Debug.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"

#include "SDL_pixels.h"

// ******************************************************************************
//
// Video Object SGP Module
//
// Video Objects are used to contain any imagery which requires blitting. The
// data is contained within a Direct Draw surface. Palette information is in
// both a Direct Draw Palette and a 16BPP palette structure for 8->16 BPP Blits.
// Blitting is done via Direct Draw as well as custum blitters. Regions are
// used to define local coordinates within the surface
//
// Second Revision: Dec 10, 1996, Andrew Emmons
//
// *******************************************************************************

static SGPVObject *gpVObjectHead = 0;

SGPVObject::SGPVObject(SGPImage const *const img)
    : flags_(), palette16_(), current_shade_(), ppZStripInfo(), next_(gpVObjectHead) {
  memset(&pShades[0], 0, sizeof(pShades));

  if (!(img->fFlags & IMAGE_TRLECOMPRESSED)) {
    throw std::runtime_error("Image for video object creation must be TRLE compressed");
  }

  ETRLEData TempETRLEData;
  GetETRLEImageData(img, &TempETRLEData);

  subregion_count_ = TempETRLEData.usNumberOfObjects;
  etrle_object_ = TempETRLEData.pETRLEObject;
  pix_data_ = static_cast<uint8_t *>(TempETRLEData.pPixData);
  pix_data_size_ = TempETRLEData.uiSizePixData;
  bit_depth_ = img->ubBitDepth;

  if (img->ubBitDepth == 8) {
    // create palette
    const SGPPaletteEntry *const src_pal = img->pPalette;
    Assert(src_pal != NULL);

    SGPPaletteEntry *const pal = palette_.Allocate(256);
    memcpy(pal, src_pal, sizeof(*pal) * 256);

    palette16_ = Create16BPPPalette(pal);
    current_shade_ = palette16_;
  }

  gpVObjectHead = this;
}

SGPVObject::~SGPVObject() {
  for (SGPVObject **anchor = &gpVObjectHead;; anchor = &(*anchor)->next_) {
    if (*anchor != this) continue;
    *anchor = next_;
    break;
  }

  DestroyPalettes();

  if (pix_data_) MemFree(pix_data_);
  if (etrle_object_) MemFree(etrle_object_);

  if (ppZStripInfo != NULL) {
    for (uint32_t usLoop = 0; usLoop < SubregionCount(); usLoop++) {
      if (ppZStripInfo[usLoop] != NULL) {
        MemFree(ppZStripInfo[usLoop]->pbZChange);
        MemFree(ppZStripInfo[usLoop]);
      }
    }
    MemFree(ppZStripInfo);
  }
}

void SGPVObject::CurrentShade(size_t const idx) {
  if (idx >= lengthof(pShades) || !pShades[idx]) {
    throw std::logic_error("Tried to set invalid video object shade");
  }
  current_shade_ = pShades[idx];
}

ETRLEObject const &SGPVObject::SubregionProperties(size_t const idx) const {
  if (idx >= SubregionCount()) {
    throw std::logic_error("Tried to access invalid subregion in video object");
  }
  return etrle_object_[idx];
}

uint8_t const *SGPVObject::PixData(ETRLEObject const &e) const {
  return &pix_data_[e.uiDataOffset];
}

#define COMPRESS_TRANSPARENT 0x80
#define COMPRESS_RUN_MASK 0x7F

uint8_t SGPVObject::GetETRLEPixelValue(uint16_t const usETRLEIndex, uint16_t const usX,
                                       uint16_t const usY) const {
  ETRLEObject const &pETRLEObject = SubregionProperties(usETRLEIndex);

  if (usX >= pETRLEObject.usWidth || usY >= pETRLEObject.usHeight) {
    throw std::logic_error("Tried to get pixel from invalid coordinate");
  }

  // Assuming everything's okay, go ahead and look...
  uint8_t const *pCurrent = PixData(pETRLEObject);

  // Skip past all uninteresting scanlines
  for (uint16_t usLoopY = 0; usLoopY < usY; usLoopY++) {
    while (*pCurrent != 0) {
      if (*pCurrent & COMPRESS_TRANSPARENT) {
        pCurrent++;
      } else {
        pCurrent += *pCurrent & COMPRESS_RUN_MASK;
      }
    }
  }

  // Now look in this scanline for the appropriate byte
  uint16_t usLoopX = 0;
  do {
    uint16_t ubRunLength = *pCurrent & COMPRESS_RUN_MASK;

    if (*pCurrent & COMPRESS_TRANSPARENT) {
      if (usLoopX + ubRunLength >= usX) return 0;
      pCurrent++;
    } else {
      if (usLoopX + ubRunLength >= usX) {
        // skip to the correct byte; skip at least 1 to get past the byte
        // defining the run
        pCurrent += (usX - usLoopX) + 1;
        return *pCurrent;
      } else {
        pCurrent += ubRunLength + 1;
      }
    }
    usLoopX += ubRunLength;
  } while (usLoopX < usX);

  throw std::logic_error("Inconsistent video object data");
}

/* Destroys the palette tables of a video object. All memory is deallocated, and
 * the pointers set to NULL. Be careful not to try and blit this object until
 * new tables are calculated, or things WILL go boom. */
void SGPVObject::DestroyPalettes() {
  FOR_EACH(uint16_t *, i, pShades) {
    if (flags_ & SHADETABLE_SHARED) continue;
    uint16_t *const p = *i;
    if (!p) continue;
    if (palette16_ == p) palette16_ = 0;
    *i = 0;
    MemFree(p);
  }

  if (uint16_t *const p = palette16_) {
    palette16_ = 0;
    MemFree(p);
  }

  current_shade_ = 0;
}

void SGPVObject::ShareShadetables(SGPVObject *const other) {
  flags_ |= SHADETABLE_SHARED;
  for (size_t i = 0; i < lengthof(pShades); ++i) {
    pShades[i] = other->pShades[i];
  }
}

void InitializeVideoObjectManager() {
  // Shouldn't be calling this if the video object manager already exists.
  // Call shutdown first...
  Assert(gpVObjectHead == NULL);
  gpVObjectHead = NULL;
}

void ShutdownVideoObjectManager() {
  while (gpVObjectHead) {
    delete gpVObjectHead;
  }
}

SGPVObject *AddStandardVideoObjectFromHImage(SGPImage *const img) { return new SGPVObject(img); }

SGPVObject *AddStandardVideoObjectFromFile(const char *const ImageFile) {
  AutoSGPImage hImage(CreateImage(ImageFile, IMAGE_ALLIMAGEDATA));
  return AddStandardVideoObjectFromHImage(hImage);
}

void BltVideoObject(SGPVSurface *const dst, SGPVObject const *const src,
                    uint16_t const usRegionIndex, int32_t const iDestX, int32_t const iDestY) {
  Assert(src->BPP() == 8);
  Assert(dst->BPP() == 16);

  SGPVSurface::Lock l(dst);
  uint16_t *const pBuffer = l.Buffer<uint16_t>();
  uint32_t const uiPitch = l.Pitch();

  if (BltIsClipped(src, iDestX, iDestY, usRegionIndex, &ClippingRect)) {
    Blt8BPPDataTo16BPPBufferTransparentClip(pBuffer, uiPitch, src, iDestX, iDestY, usRegionIndex,
                                            &ClippingRect);
  } else {
    Blt8BPPDataTo16BPPBufferTransparent(pBuffer, uiPitch, src, iDestX, iDestY, usRegionIndex);
  }
}

void BltVideoObjectOutline(SGPVSurface *const dst, SGPVObject const *const hSrcVObject,
                           uint16_t const usIndex, int32_t const iDestX, int32_t const iDestY,
                           int16_t const s16BPPColor) {
  SGPVSurface::Lock l(dst);
  uint16_t *const pBuffer = l.Buffer<uint16_t>();
  uint32_t const uiPitch = l.Pitch();

  if (BltIsClipped(hSrcVObject, iDestX, iDestY, usIndex, &ClippingRect)) {
    Blt8BPPDataTo16BPPBufferOutlineClip(pBuffer, uiPitch, hSrcVObject, iDestX, iDestY, usIndex,
                                        s16BPPColor, &ClippingRect);
  } else {
    Blt8BPPDataTo16BPPBufferOutline(pBuffer, uiPitch, hSrcVObject, iDestX, iDestY, usIndex,
                                    s16BPPColor);
  }
}

void BltVideoObjectOutlineShadow(SGPVSurface *const dst, const SGPVObject *const src,
                                 const uint16_t usIndex, const int32_t iDestX,
                                 const int32_t iDestY) {
  SGPVSurface::Lock l(dst);
  uint16_t *const pBuffer = l.Buffer<uint16_t>();
  uint32_t const uiPitch = l.Pitch();

  if (BltIsClipped(src, iDestX, iDestY, usIndex, &ClippingRect)) {
    Blt8BPPDataTo16BPPBufferOutlineShadowClip(pBuffer, uiPitch, src, iDestX, iDestY, usIndex,
                                              &ClippingRect);
  } else {
    Blt8BPPDataTo16BPPBufferOutlineShadow(pBuffer, uiPitch, src, iDestX, iDestY, usIndex);
  }
}

void BltVideoObjectOnce(SGPVSurface *const dst, char const *const filename, uint16_t const region,
                        int32_t const x, int32_t const y) {
  AutoSGPVObject vo(AddVideoObjectFromFile(filename));
  BltVideoObject(dst, vo, region, x, y);
}
