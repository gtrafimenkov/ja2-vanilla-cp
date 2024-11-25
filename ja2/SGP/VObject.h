// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __VOBJECT_H
#define __VOBJECT_H

#include "SGP/AutoPtr.h"
#include "SGP/Buffer.h"
#include "SGP/Types.h"

// Defines for HVOBJECT limits
#define HVOBJECT_SHADE_TABLES 48

// Z-buffer info structure for properly assigning Z values
struct ZStripInfo {
  int8_t bInitialZChange;      // difference in Z value between the leftmost and base
                               // strips
  uint8_t ubFirstZStripWidth;  // # of pixels in the leftmost strip
  uint8_t ubNumberOfZChanges;  // number of strips (after the first)
  int8_t *pbZChange;           // change to the Z value in each strip (after the first)
};

// This definition mimics what is found in WINDOWS.H ( for Direct Draw
// compatiblity ) From RGB to COLORVAL
#define FROMRGB(r, g, b) \
  ((uint32_t)(((uint8_t)(r) | ((uint16_t)(g) << 8)) | (((uint32_t)(uint8_t)(b)) << 16)))

// This structure is a video object.
// The video object contains different data based on it's type, compressed or
// not
class SGPVObject {
 public:
  SGPVObject(SGPImage const *);
  ~SGPVObject();

  uint8_t BPP() const { return bit_depth_; }

  SGPPaletteEntry const *Palette() const { return palette_; }

  uint16_t const *Palette16() const { return palette16_; }

  uint16_t const *CurrentShade() const { return current_shade_; }

  // Set the current object shade table
  void CurrentShade(size_t idx);

  uint16_t SubregionCount() const { return subregion_count_; }

  ETRLEObject const &SubregionProperties(size_t idx) const;

  uint8_t const *PixData(ETRLEObject const &) const;

  /* Given a ETRLE image index, retrieves the value of the pixel located at
   * the given image coordinates. The value returned is an 8-bit palette index
   */
  uint8_t GetETRLEPixelValue(uint16_t usETLREIndex, uint16_t usX, uint16_t usY) const;

  // Deletes the 16-bit palette tables
  void DestroyPalettes();

  void ShareShadetables(SGPVObject *);

  enum Flags { NONE = 0, SHADETABLE_SHARED = 1U << 0 };

 private:
  Flags flags_;                           // Special flags
  uint32_t pix_data_size_;                // ETRLE data size
  SGP::Buffer<SGPPaletteEntry> palette_;  // 8BPP Palette
  uint16_t *palette16_;                   // A 16BPP palette used for 8->16 blits

  uint8_t *pix_data_;          // ETRLE pixel data
  ETRLEObject *etrle_object_;  // Object offset data etc
 public:
  uint16_t *pShades[HVOBJECT_SHADE_TABLES];  // Shading tables
 private:
  uint16_t const *current_shade_;

 public:
  ZStripInfo **ppZStripInfo;  // Z-value strip info arrays

 private:
  uint16_t subregion_count_;  // Total number of objects
  uint8_t bit_depth_;         // BPP

 public:
  SGPVObject *next_;
};
ENUM_BITSET(SGPVObject::Flags)

// Creates a list to contain video objects
void InitializeVideoObjectManager();

// Deletes any video object placed into list
void ShutdownVideoObjectManager();

// Creates and adds a video object to list
SGPVObject *AddStandardVideoObjectFromHImage(SGPImage *);
SGPVObject *AddStandardVideoObjectFromFile(const char *ImageFile);
#define AddVideoObjectFromHImage(a) AddStandardVideoObjectFromHImage(a)
#define AddVideoObjectFromFile(a) AddStandardVideoObjectFromFile(a)

// Removes a video object
static inline void DeleteVideoObject(SGPVObject *const vo) { delete vo; }

// Blits a video object to another video object
void BltVideoObject(SGPVSurface *dst, SGPVObject const *src, uint16_t usRegionIndex, int32_t iDestX,
                    int32_t iDestY);

void BltVideoObjectOutline(SGPVSurface *dst, SGPVObject const *src, uint16_t usIndex,
                           int32_t iDestX, int32_t iDestY, int16_t s16BPPColor);
void BltVideoObjectOutlineShadow(SGPVSurface *dst, SGPVObject const *src, uint16_t usIndex,
                                 int32_t iDestX, int32_t iDestY);

/* Loads a video object, blits it once and frees it */
void BltVideoObjectOnce(SGPVSurface *dst, char const *filename, uint16_t region, int32_t x,
                        int32_t y);

typedef SGP::AutoPtr<SGPVObject> AutoSGPVObject;

#endif
