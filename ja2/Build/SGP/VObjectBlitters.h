#ifndef __VOBJECT_BLITTERS
#define __VOBJECT_BLITTERS

#include "SGP/Types.h"

extern SGPRect ClippingRect;
extern uint32_t guiTranslucentMask;

extern void SetClippingRect(SGPRect *clip);
void GetClippingRect(SGPRect *clip);

BOOLEAN BltIsClipped(const SGPVObject *hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex,
                     const SGPRect *clipregion);
char BltIsClippedOrOffScreen(HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex,
                             SGPRect *clipregion);

/* Allocate and initialize a Z-buffer for use with the Z-buffer blitters.
 * Doesn't really do much except allocate a chunk of memory, and zero it. */
uint16_t *InitZBuffer(uint32_t width, uint32_t height);

/* Free the memory allocated for the Z-buffer. */
void ShutdownZBuffer(uint16_t *pBuffer);

// translucency blitters
void Blt8BPPDataTo16BPPBufferTransZTranslucent(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                               uint16_t *pZBuffer, uint16_t usZValue,
                                               HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                               uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferTransZNBTranslucent(uint16_t *buf, uint32_t uiDestPitchBYTES,
                                                 uint16_t *zbuf, uint16_t zval,
                                                 HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                 uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferTransZNBClipTranslucent(uint16_t *buf, uint32_t uiDestPitchBYTES,
                                                     uint16_t *zbuf, uint16_t zval,
                                                     HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                     uint16_t usIndex, SGPRect const *clipregion);

void Blt8BPPDataTo16BPPBufferMonoShadowClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                            HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                            uint16_t usIndex, SGPRect *clipregion,
                                            uint16_t usForeground, uint16_t usBackground,
                                            uint16_t usShadow);

void Blt8BPPDataTo16BPPBufferTransZ(uint16_t *buf, uint32_t uiDestPitchBYTES, uint16_t *zbuf,
                                    uint16_t zval, HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                    uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferTransZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                      uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                      int32_t iX, int32_t iY, uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferTransZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                        uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                        int32_t iX, int32_t iY, uint16_t usIndex,
                                        SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferTransZNBClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                          uint16_t *pZBuffer, uint16_t usZValue,
                                          HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                          uint16_t usIndex, SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferTransShadowZ(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                          uint16_t *pZBuffer, uint16_t usZValue,
                                          HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                          uint16_t usIndex, const uint16_t *p16BPPPalette);
void Blt8BPPDataTo16BPPBufferTransShadowClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                             HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                             uint16_t usIndex, SGPRect *clipregion,
                                             const uint16_t *p16BPPPalette);
void Blt8BPPDataTo16BPPBufferTransShadowZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                            uint16_t *pZBuffer, uint16_t usZValue,
                                            HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                            uint16_t usIndex, const uint16_t *p16BPPPalette);
void Blt8BPPDataTo16BPPBufferShadowZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                       uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                       int32_t iX, int32_t iY, uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferShadowZNBClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                           uint16_t *pZBuffer, uint16_t usZValue,
                                           HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                           uint16_t usIndex, SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferShadowZ(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                     uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                     int32_t iX, int32_t iY, uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferShadowZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                         uint16_t *pZBuffer, uint16_t usZValue,
                                         HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                         uint16_t usIndex, SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferTransShadowZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                              uint16_t *pZBuffer, uint16_t usZValue,
                                              HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                              uint16_t usIndex, SGPRect *clipregion,
                                              const uint16_t *p16BPPPalette);
void Blt8BPPDataTo16BPPBufferTransShadowZNBClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                uint16_t *pZBuffer, uint16_t usZValue,
                                                HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                uint16_t usIndex, SGPRect *clipregion,
                                                const uint16_t *p16BPPPalette);

// Next blitters are for blitting mask as intensity
void Blt8BPPDataTo16BPPBufferIntensityZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                          uint16_t *pZBuffer, uint16_t usZValue,
                                          HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                          uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferIntensityZ(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                        uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                        int32_t iX, int32_t iY, uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferIntensityZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                            uint16_t *pZBuffer, uint16_t usZValue,
                                            HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                            uint16_t usIndex, SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferIntensityClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                           HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                           uint16_t usIndex, SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferIntensity(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                       HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                       uint16_t usIndex);

void Blt8BPPDataTo16BPPBufferTransparentClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                             const SGPVObject *hSrcVObject, int32_t iX, int32_t iY,
                                             uint16_t usIndex, const SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferTransparent(uint16_t *buf, uint32_t uiDestPitchBYTES,
                                         SGPVObject const *hSrcVObject, int32_t iX, int32_t iY,
                                         uint16_t usIndex);

void Blt8BPPDataTo16BPPBufferTransShadow(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                         HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                         uint16_t usIndex, const uint16_t *p16BPPPalette);

void Blt8BPPDataTo16BPPBufferShadowClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                        HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                        uint16_t usIndex, SGPRect *clipregion);

void Blt16BPPTo16BPP(uint16_t *pDest, uint32_t uiDestPitch, uint16_t *pSrc, uint32_t uiSrcPitch,
                     int32_t iDestXPos, int32_t iDestYPos, int32_t iSrcXPos, int32_t iSrcYPos,
                     uint32_t uiWidth, uint32_t uiHeight);

void Blt16BPPBufferHatchRect(uint16_t *pBuffer, uint32_t uiDestPitchBYTES, SGPRect *area);
void Blt16BPPBufferLooseHatchRectWithColor(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                           SGPRect *area, uint16_t usColor);

/* Filter a rectangular area with the given filter table.  This is used for
 * shading. */
void Blt16BPPBufferFilterRect(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                              const uint16_t *filter_table, SGPRect *area);

void Blt8BPPDataTo16BPPBufferShadow(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex);

void Blt8BPPDataTo16BPPBuffer(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                              SGPVSurface *hSrcVSurface, uint8_t *pSrcBuffer, int32_t iX,
                              int32_t iY);
void Blt8BPPDataSubTo16BPPBuffer(uint16_t *buf, uint32_t uiDestPitchBYTES,
                                 SGPVSurface *hSrcVSurface, uint8_t *pSrcBuffer, uint32_t src_pitch,
                                 int32_t iX, int32_t iY, SGPBox const *rect);

// Blits from flat 8bpp source, to 16bpp dest, divides in half
void Blt8BPPDataTo16BPPBufferHalf(uint16_t *dst_buf, uint32_t uiDestPitchBYTES,
                                  SGPVSurface *src_surface, uint8_t const *src_buf,
                                  uint32_t src_pitch, int32_t x, int32_t y, SGPBox const *rect);

// ATE: New blitters for showing an outline at color 254
void Blt8BPPDataTo16BPPBufferOutline(uint16_t *buf, uint32_t uiDestPitchBYTES,
                                     SGPVObject const *hSrcVObject, int32_t iX, int32_t iY,
                                     uint16_t usIndex, int16_t outline);
void Blt8BPPDataTo16BPPBufferOutlineClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                         const SGPVObject *hSrcVObject, int32_t iX, int32_t iY,
                                         uint16_t usIndex, int16_t s16BPPColor,
                                         const SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferOutlineZ(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                      uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                      int32_t iX, int32_t iY, uint16_t usIndex,
                                      int16_t s16BPPColor);
void Blt8BPPDataTo16BPPBufferOutlineShadow(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                           const SGPVObject *hSrcVObject, int32_t iX, int32_t iY,
                                           uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferOutlineShadowClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                               const SGPVObject *hSrcVObject, int32_t iX,
                                               int32_t iY, uint16_t usIndex,
                                               const SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferOutlineZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                        uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                        int32_t iX, int32_t iY, uint16_t usIndex);
void Blt8BPPDataTo16BPPBufferOutlineZPixelateObscured(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                      uint16_t *pZBuffer, uint16_t usZValue,
                                                      HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                      uint16_t usIndex, int16_t s16BPPColor);
void Blt8BPPDataTo16BPPBufferOutlineZPixelateObscuredClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, int16_t s16BPPColor,
    const SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferOutlineZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                          uint16_t *pZBuffer, uint16_t usZValue,
                                          HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                          uint16_t usIndex, int16_t s16BPPColor,
                                          const SGPRect *clipregion);

// ATE: New blitter for included shadow, but pixellate if obscured by z
void Blt8BPPDataTo16BPPBufferTransShadowZNBObscured(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                    uint16_t *pZBuffer, uint16_t usZValue,
                                                    HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                    uint16_t usIndex,
                                                    const uint16_t *p16BPPPalette);

void Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion,
    const uint16_t *p16BPPPalette);
void Blt8BPPDataTo16BPPBufferTransZClipPixelateObscured(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion);
void Blt8BPPDataTo16BPPBufferTransZPixelateObscured(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                    uint16_t *pZBuffer, uint16_t usZValue,
                                                    HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                    uint16_t usIndex);

#endif
