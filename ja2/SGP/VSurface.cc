// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/VSurface.h"

#include <stdexcept>

#include "SGP/Debug.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/SGP.h"
#include "SGP/Shading.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/Video.h"

#include "SDL_pixels.h"
#include "SDL_surface.h"

int _LockSurface(SDL_Surface *surface) { return SDL_LockSurface(surface); }
void _UnlockSurface(SDL_Surface *surface) { SDL_UnlockSurface(surface); }

int _Surface_GetPitch(SDL_Surface *surface) { return surface->pitch; }
void *_Surface_GetPixels(SDL_Surface *surface) { return surface->pixels; };

extern SGPVSurface *gpVSurfaceHead;

SGPVSurface::SGPVSurface(uint16_t const w, uint16_t const h, uint8_t const bpp)
    : p16BPPPalette(), next_(gpVSurfaceHead) {
  Assert(w > 0);
  Assert(h > 0);

  SDL_Surface *s;
  switch (bpp) {
    case 8:
      s = SDL_CreateRGBSurface(0, w, h, bpp, 0, 0, 0, 0);
      break;

    case 16: {
      SDL_PixelFormat const *f = SDL_AllocFormat(SDL_PIXELFORMAT_RGB565);
      s = SDL_CreateRGBSurface(0, w, h, bpp, f->Rmask, f->Gmask, f->Bmask, f->Amask);
      break;
    }

    default:
      throw std::logic_error("Tried to create video surface with invalid bpp, must be 8 or 16.");
  }
  if (!s) throw std::runtime_error("Failed to create SDL surface");
  surface_ = s;
  gpVSurfaceHead = this;
}

SGPVSurface::SGPVSurface(SDL_Surface *const s)
    : surface_(s), p16BPPPalette(), next_(gpVSurfaceHead) {
  gpVSurfaceHead = this;
}

uint16_t SGPVSurface::Width() const { return surface_->w; }
uint16_t SGPVSurface::Height() const { return surface_->h; }
uint8_t SGPVSurface::BPP() const { return surface_->format->BitsPerPixel; }

SGPVSurface::~SGPVSurface() {
  for (SGPVSurface **anchor = &gpVSurfaceHead;; anchor = &(*anchor)->next_) {
    if (*anchor != this) continue;
    *anchor = next_;
    break;
  }

  if (p16BPPPalette) MemFree(p16BPPPalette);
}

void SGPVSurface::SetPalette(const SGPPaletteEntry *const src_pal) {
  // Create palette object if not already done so
  if (!palette_) palette_.Allocate(256);
  SGPPaletteEntry *const p = palette_;
  for (uint32_t i = 0; i < 256; i++) {
    p[i] = src_pal[i];
  }

  if (p16BPPPalette != NULL) MemFree(p16BPPPalette);
  p16BPPPalette = Create16BPPPalette(src_pal);
}

void SGPVSurface::SetTransparency(const COLORVAL colour) {
  Uint32 colour_key;
  switch (BPP()) {
    case 8:
      colour_key = colour;
      break;
    case 16:
      colour_key = Get16BPPColor(colour);
      break;

    default:
      abort();  // HACK000E
  }
  SDL_SetColorKey(surface_, SDL_TRUE, colour_key);
}

void SGPVSurface::Fill(const uint16_t colour) { SDL_FillRect(surface_, NULL, colour); }

SGPVSurfaceAuto::SGPVSurfaceAuto(uint16_t w, uint16_t h, uint8_t bpp) : SGPVSurface(w, h, bpp) {}

SGPVSurfaceAuto::SGPVSurfaceAuto(SDL_Surface *surface) : SGPVSurface(surface) {}

SGPVSurfaceAuto::~SGPVSurfaceAuto() {
  if (surface_) {
    SDL_FreeSurface(surface_);
  }
}

static void InternalShadowVideoSurfaceRect(SGPVSurface *const dst, int32_t X1, int32_t Y1,
                                           int32_t X2, int32_t Y2,
                                           const uint16_t *const filter_table) {
  if (X1 < 0) X1 = 0;
  if (X2 < 0) return;

  if (Y2 < 0) return;
  if (Y1 < 0) Y1 = 0;

  if (X2 >= dst->Width()) X2 = dst->Width() - 1;
  if (Y2 >= dst->Height()) Y2 = dst->Height() - 1;

  if (X1 >= dst->Width()) return;
  if (Y1 >= dst->Height()) return;

  if (X2 - X1 <= 0) return;
  if (Y2 - Y1 <= 0) return;

  SGPRect area;
  area.iTop = Y1;
  area.iBottom = Y2;
  area.iLeft = X1;
  area.iRight = X2;

  SGPVSurface::Lock ldst(dst);
  Blt16BPPBufferFilterRect(ldst.Buffer<uint16_t>(), ldst.Pitch(), filter_table, &area);
}

void SGPVSurface::ShadowRect(int32_t const x1, int32_t const y1, int32_t const x2,
                             int32_t const y2) {
  InternalShadowVideoSurfaceRect(this, x1, y1, x2, y2, ShadeTable);
}

void SGPVSurface::ShadowRectUsingLowPercentTable(int32_t const x1, int32_t const y1,
                                                 int32_t const x2, int32_t const y2) {
  InternalShadowVideoSurfaceRect(this, x1, y1, x2, y2, IntensityTable);
}

static void DeletePrimaryVideoSurfaces();

SGPVSurface *g_back_buffer;
SGPVSurfaceAuto *g_frame_buffer;
SGPVSurfaceAuto *g_mouse_buffer;

#undef AddVideoSurface
#undef AddVideoSurfaceFromFile

SGPVSurfaceAuto *AddVideoSurface(uint16_t Width, uint16_t Height, uint8_t BitDepth) {
  SGPVSurfaceAuto *const vs = new SGPVSurfaceAuto(Width, Height, BitDepth);
  return vs;
}

SGPVSurfaceAuto *AddVideoSurfaceFromFile(const char *const Filename) {
  AutoSGPImage img(CreateImage(Filename, IMAGE_ALLIMAGEDATA));

  SGPVSurfaceAuto *const vs = new SGPVSurfaceAuto(img->usWidth, img->usHeight, img->ubBitDepth);

  uint8_t const dst_bpp = vs->BPP();
  uint32_t buffer_bpp;
  switch (dst_bpp) {
    case 8:
      buffer_bpp = BUFFER_8BPP;
      break;
    case 16:
      buffer_bpp = BUFFER_16BPP;
      break;
    default:
      throw std::logic_error("Invalid bpp");
  }

  {
    SGPVSurface::Lock l(vs);
    uint8_t *const dst = l.Buffer<uint8_t>();
    uint16_t const pitch = l.Pitch() / (dst_bpp / 8);  // pitch in pixels
    SGPBox const box = {0, 0, img->usWidth, img->usHeight};
    BOOLEAN const Ret = CopyImageToBuffer(img, buffer_bpp, dst, pitch, vs->Height(), 0, 0, &box);
    if (!Ret) {
      DebugMsg(TOPIC_VIDEOSURFACE, DBG_LEVEL_2, "Error Occured Copying SGPImage to video surface");
    }
  }

  if (img->ubBitDepth == 8) vs->SetPalette(img->pPalette);

  return vs;
}

#define RECORD(cs, name) ((void)0)

void BltVideoSurfaceHalf(SGPVSurface *const dst, SGPVSurface *const src, int32_t const DestX,
                         int32_t const DestY, SGPBox const *const src_rect) {
  SGPVSurface::Lock lsrc(src);
  SGPVSurface::Lock ldst(dst);
  uint8_t *const SrcBuf = lsrc.Buffer<uint8_t>();
  uint32_t const SrcPitchBYTES = lsrc.Pitch();
  uint16_t *const DestBuf = ldst.Buffer<uint16_t>();
  uint32_t const DestPitchBYTES = ldst.Pitch();
  Blt8BPPDataTo16BPPBufferHalf(DestBuf, DestPitchBYTES, src, SrcBuf, SrcPitchBYTES, DestX, DestY,
                               src_rect);
}

void ColorFillVideoSurfaceArea(SGPVSurface *const dst, int32_t iDestX1, int32_t iDestY1,
                               int32_t iDestX2, int32_t iDestY2, const uint16_t Color16BPP) {
  SGPRect Clip;
  GetClippingRect(&Clip);

  if (iDestX1 < Clip.iLeft) iDestX1 = Clip.iLeft;
  if (iDestX1 > Clip.iRight) return;

  if (iDestX2 > Clip.iRight) iDestX2 = Clip.iRight;
  if (iDestX2 < Clip.iLeft) return;

  if (iDestY1 < Clip.iTop) iDestY1 = Clip.iTop;
  if (iDestY1 > Clip.iBottom) return;

  if (iDestY2 > Clip.iBottom) iDestY2 = Clip.iBottom;
  if (iDestY2 < Clip.iTop) return;

  if (iDestX2 <= iDestX1 || iDestY2 <= iDestY1) return;

  SDL_Rect Rect;
  Rect.x = iDestX1;
  Rect.y = iDestY1;
  Rect.w = iDestX2 - iDestX1;
  Rect.h = iDestY2 - iDestY1;
  SDL_FillRect(dst->surface_, &Rect, Color16BPP);
}

// Will drop down into user-defined blitter if 8->16 BPP blitting is being done
void BltVideoSurface(SGPVSurface *const dst, SGPVSurface *const src, int32_t const iDestX,
                     int32_t const iDestY, SGPBox const *const src_box) {
  Assert(dst);
  Assert(src);

  const uint8_t src_bpp = src->BPP();
  const uint8_t dst_bpp = dst->BPP();
  if (src_bpp == dst_bpp) {
    SDL_Rect *src_rect = 0;
    SDL_Rect r;
    if (src_box) {
      r.x = src_box->x;
      r.y = src_box->y;
      r.w = src_box->w;
      r.h = src_box->h;
      src_rect = &r;
    }

    SDL_Rect dstrect;
    dstrect.x = iDestX;
    dstrect.y = iDestY;
    SDL_BlitSurface(src->surface_, src_rect, dst->surface_, &dstrect);
#if defined __GNUC__ && defined i386
    __asm__ __volatile__("cld");  // XXX HACK000D
#endif
  } else if (src_bpp < dst_bpp) {
    SGPBox const *src_rect = src_box;
    SGPBox r;
    if (!src_rect) {
      // Check Sizes, SRC size MUST be <= DEST size
      if (dst->Height() < src->Height()) {
        DebugMsg(TOPIC_VIDEOSURFACE, DBG_LEVEL_2,
                 "Incompatible height size given in Video Surface blit");
        return;
      }
      if (dst->Width() < src->Width()) {
        DebugMsg(TOPIC_VIDEOSURFACE, DBG_LEVEL_2,
                 "Incompatible height size given in Video Surface blit");
        return;
      }

      r.x = 0;
      r.y = 0;
      r.w = src->Width();
      r.h = src->Height();
      src_rect = &r;
    }

    SGPVSurface::Lock lsrc(src);
    SGPVSurface::Lock ldst(dst);
    uint8_t *const s_buf = lsrc.Buffer<uint8_t>();
    uint32_t const spitch = lsrc.Pitch();
    uint16_t *const d_buf = ldst.Buffer<uint16_t>();
    uint32_t const dpitch = ldst.Pitch();
    Blt8BPPDataSubTo16BPPBuffer(d_buf, dpitch, src, s_buf, spitch, iDestX, iDestY, src_rect);
  } else {
    DebugMsg(TOPIC_VIDEOSURFACE, DBG_LEVEL_2,
             "Incompatible BPP values with src and dest Video Surfaces for "
             "blitting");
  }
}

void BltStretchVideoSurface(SGPVSurface *const dst, SGPVSurface const *const src,
                            SGPBox const *const src_rect, SGPBox const *const dst_rect) {
  if (dst->BPP() != 16 || src->BPP() != 16) return;

  SDL_Surface const *const ssurface = src->surface_;
  SDL_Surface *const dsurface = dst->surface_;

  const uint32_t s_pitch = ssurface->pitch >> 1;
  const uint32_t d_pitch = dsurface->pitch >> 1;
  uint16_t const *os = (const uint16_t *)ssurface->pixels + s_pitch * src_rect->y + src_rect->x;
  uint16_t *d = (uint16_t *)dsurface->pixels + d_pitch * dst_rect->y + dst_rect->x;

  uint32_t const width = dst_rect->w;
  uint32_t const height = dst_rect->h;
  uint32_t const dx = src_rect->w;
  uint32_t const dy = src_rect->h;
  uint32_t py = 0;
  // GT: This condition is very strange.
  // Somehow it is used to to test if transparency color key was configured.
  if (ssurface->flags & SDL_TRUE) {
    const uint16_t transparency_key = 0;
    for (uint32_t iy = 0; iy < height; ++iy) {
      const uint16_t *s = os;
      uint32_t px = 0;
      for (uint32_t ix = 0; ix < width; ++ix) {
        if (*s != transparency_key) *d = *s;
        ++d;
        px += dx;
        for (; px >= width; px -= width) ++s;
      }
      d += d_pitch - width;
      py += dy;
      for (; py >= height; py -= height) os += s_pitch;
    }
  } else {
    for (uint32_t iy = 0; iy < height; ++iy) {
      const uint16_t *s = os;
      uint32_t px = 0;
      for (uint32_t ix = 0; ix < width; ++ix) {
        *d++ = *s;
        px += dx;
        for (; px >= width; px -= width) ++s;
      }
      d += d_pitch - width;
      py += dy;
      for (; py >= height; py -= height) os += s_pitch;
    }
  }
}

void BltVideoSurfaceOnce(SGPVSurface *const dst, const char *const filename, int32_t const x,
                         int32_t const y) {
  SGP::AutoPtr<SGPVSurfaceAuto> src(AddVideoSurfaceFromFile(filename));
  BltVideoSurface(dst, src, x, y, NULL);
}

/** Draw image on the video surface stretching the image if necessary. */
void BltVideoSurfaceOnceWithStretch(SGPVSurface *const dst, const char *const filename) {
  SGP::AutoPtr<SGPVSurfaceAuto> src(AddVideoSurfaceFromFile(filename));
  FillVideoSurfaceWithStretch(dst, src);
}

/** Fill video surface with another one with stretch. */
void FillVideoSurfaceWithStretch(SGPVSurface *const dst, SGPVSurface *const src) {
  SGPBox srcRec;
  SGPBox dstRec;
  srcRec.set(0, 0, src->Width(), src->Height());
  dstRec.set(0, 0, dst->Width(), dst->Height());
  BltStretchVideoSurface(dst, src, &srcRec, &dstRec);
}
