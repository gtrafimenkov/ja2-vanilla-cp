#include "SGP/VObjectBlitters.h"

#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Local.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/Shading.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/WCheck.h"

SGPRect ClippingRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
// 555      565
uint32_t guiTranslucentMask = 0x3def;  // 0x7bef;		// mask for halving 5,6,5

/* Difference or Zero */
template <typename T>
static inline T DoZ(T const a, T const b) {
  return a > b ? a - b : 0;
}

/* Blit an image into the destination buffer, using an ETRLE brush as a source,
 * and a 16-bit buffer as a destination. As it is blitting, it checks the Z
 * value of the ZBuffer, and if the pixel's Z level is below that of the current
 * pixel, it is written on, and the Z value is NOT updated to the current value,
 * for any non-transparent pixels. The Z-buffer is 16 bit, and must be the same
 * dimensions (including pitch) as the destination.
 * Blits every second pixel ("Translucents"). */
void Blt8BPPDataTo16BPPBufferTransZNBClipTranslucent(uint16_t *const buf,
                                                     uint32_t const uiDestPitchBYTES,
                                                     uint16_t *const zbuf, uint16_t const zval,
                                                     HVOBJECT const hSrcVObject, int32_t const iX,
                                                     int32_t const iY, uint16_t const usIndex,
                                                     SGPRect const *clipregion) {
  Assert(hSrcVObject);
  Assert(buf);

  // Get offsets from index into structure.
  ETRLEObject const &e = hSrcVObject->SubregionProperties(usIndex);
  int32_t const height = e.usHeight;
  int32_t const width = e.usWidth;

  // Add to start position of dest buffer.
  int32_t const x = iX + e.sOffsetX;
  int32_t const y = iY + e.sOffsetY;

  if (!clipregion) clipregion = &ClippingRect;

  /* Calculate rows hanging off each side of the screen and check if the whole
   * thing is clipped. */
  int32_t const left_skip = DoZ(clipregion->iLeft, x);
  if (left_skip >= width) return;
  int32_t top_skip = DoZ(clipregion->iTop, y);
  if (top_skip >= height) return;
  int32_t const right_skip = DoZ(x + width, clipregion->iRight);
  if (right_skip >= width) return;
  int32_t const bottom_skip = DoZ(y + height, clipregion->iBottom);
  if (bottom_skip >= height) return;

  // Calculate the remaining rows and columns to blit.
  int32_t const blit_length = (int32_t)width - left_skip - right_skip;
  int32_t blit_height = (int32_t)height - top_skip - bottom_skip;

  uint32_t const pitch = uiDestPitchBYTES / 2;
  uint8_t const *src = hSrcVObject->PixData(e);
  uint16_t *dst = buf + pitch * (y + top_skip) + (x + left_skip);
  uint16_t const *zdst = zbuf + pitch * (y + top_skip) + (x + left_skip);
  uint16_t const *const pal = hSrcVObject->CurrentShade();
  uint32_t line_skip = pitch - blit_length;

#if 1  // XXX TODO
  for (; top_skip > 0; --top_skip) {
    for (;;) {
      uint32_t const px_count = *src++;
      if (px_count & 0x80) continue;
      if (px_count == 0) break;
      src += px_count;
    }
  }

  uint32_t const translucent_mask = guiTranslucentMask;
  do {
    int32_t ls_count;
    uint32_t px_count;
    for (ls_count = left_skip; ls_count > 0; ls_count -= px_count) {
      px_count = *src++;
      if (px_count & 0x80) {
        px_count &= 0x7F;
        if (px_count > ls_count) {
          px_count -= ls_count;
          ls_count = blit_length;
          goto BlitTransparent;
        }
      } else {
        if (px_count > ls_count) {
          src += ls_count;
          px_count -= ls_count;
          ls_count = blit_length;
          goto BlitNonTransLoop;
        }
        src += px_count;
      }
    }

    ls_count = blit_length;
    while (ls_count > 0) {
      px_count = *src++;
      if (px_count & 0x80) {  // Skip transparent pixels.
        px_count &= 0x7F;
      BlitTransparent:
        if (px_count > ls_count) px_count = ls_count;
        ls_count -= px_count;
        dst += px_count;
        zdst += px_count;
      } else {  // Blit non-transparent pixels.
      BlitNonTransLoop:
        uint32_t unblitted = 0;
        if (px_count > ls_count) {
          unblitted = px_count - ls_count;
          px_count = ls_count;
        }
        ls_count -= px_count;

        do {
          if (*zdst > zval) continue;
          *dst = (pal[*src] >> 1 & translucent_mask) + (*dst >> 1 & translucent_mask);
        } while (++src, ++dst, ++zdst, --px_count > 0);
        src += unblitted;
      }
    }

    while (*src++ != 0) {
    }  // Skip along until we hit and end-of-line marker.
    dst += line_skip;
    zdst += line_skip;
  } while (--blit_height > 0);
#else
  int32_t ls_count;
  uint32_t unblitted;

  line_skip *= 2;

  __asm {

		mov		esi, src
		mov		edi, dst
		mov		edx, pal
		xor		eax, eax
		mov		ebx, zdst
		xor		ecx, ecx

		cmp		top_skip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		top_skip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		unblitted, 0
		mov		eax, left_skip
		mov		ls_count, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ls_count
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, ls_count  // skip partial run, jump into normal loop for rest
		sub		ecx, ls_count
		mov		eax, BlitLength
		mov		ls_count, eax
		mov		unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		ls_count, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, ls_count
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, ls_count  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		ls_count, eax
		mov		unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		ls_count, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		ls_count, eax
		mov		unblitted, 0

BlitDispatch:

		cmp		ls_count, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, ls_count
		jbe		BNTrans1

		sub		ecx, ls_count
		mov		unblitted, ecx
		mov		ecx, ls_count

BNTrans1:
		sub		ls_count, ecx

BlitNTL1:
		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL2

		xor		eax, eax
		mov		edx, pal
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		shr		eax, 1
		and		eax, [guiTranslucentMask]

		xor		edx, edx
		mov		dx, [edi]
		shr		edx, 1
		and		edx, [guiTranslucentMask]

		add		eax, edx

		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, ls_count
		jbe		BTrans1

		mov		ecx, ls_count

BTrans1:

		sub		ls_count, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, line_skip
		add		ebx, line_skip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZTranslucent

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. The
Z-buffer is 16 bit, and must be the same dimensions (including Pitch) as the
destination.

        Blits every second pixel ("Translucents").

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransZTranslucent(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                               uint16_t *pZBuffer, uint16_t usZValue,
                                               HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                               uint16_t usIndex) {
  uint32_t LineSkip;
  uint8_t *DestPtr, *ZPtr;
  uint32_t uiLineFlag;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  // uint16_t const* const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));
  uiLineFlag = (iTempY & 1);

#if 1  // XXX TODO
  (void)SrcPtr;
  (void)usHeight;
  UNIMPLEMENTED
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx
        //		mov		edx, p16BPPPalette

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:
		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL5

		mov		ax, usZValue
		mov		[ebx], ax

		xor		eax, eax
		mov		edx, p16BPPPalette
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		shr		eax, 1
		and		eax, guiTranslucentMask

		xor		edx, edx
		mov		dx, [edi]
		shr		edx, 1
		and		edx, guiTranslucentMask

		add		eax, edx
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		xor		uiLineFlag, 1
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/* Blit an image into the destination buffer, using an ETRLE brush as a source,
 * and a 16-bit buffer as a destination. As it is blitting, it checks the Z
 * value of the ZBuffer, and if the pixel's Z level is below that of the current
 * pixel, it is written on, and the Z value is NOT updated to the current value,
 * for any non-transparent pixels. The Z-buffer is 16 bit, and must be the same
 * dimensions (including pitch) as the destination.
 * Blits every second pixel ("Translucents"). */
void Blt8BPPDataTo16BPPBufferTransZNBTranslucent(uint16_t *const buf,
                                                 uint32_t const uiDestPitchBYTES,
                                                 uint16_t *const zbuf, uint16_t const zval,
                                                 HVOBJECT const hSrcVObject, int32_t const iX,
                                                 int32_t const iY, uint16_t const usIndex) {
  Assert(hSrcVObject);
  Assert(buf);

  // Get offsets from index into structure.
  ETRLEObject const &e = hSrcVObject->SubregionProperties(usIndex);
  uint32_t height = e.usHeight;
  uint32_t const width = e.usWidth;

  // Add to start position of dest buffer.
  int32_t const x = iX + e.sOffsetX;
  int32_t const y = iY + e.sOffsetY;

  CHECKV(x >= 0);
  CHECKV(y >= 0);

  uint32_t const pitch = uiDestPitchBYTES / 2;
  uint8_t const *src = hSrcVObject->PixData(e);
  uint16_t *dst = buf + pitch * y + x;
  uint16_t const *zdst = zbuf + pitch * y + x;
  uint16_t const *const pal = hSrcVObject->CurrentShade();
  uint32_t line_skip = pitch - width;

#if 1  // XXX TODO
  uint32_t const translucent_mask = guiTranslucentMask;
  do {
    for (;;) {
      uint8_t data = *src++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        dst += data;
        zdst += data;
      } else {
        do {
          if (*zdst > zval) continue;
          *dst = ((pal[*src] >> 1) & translucent_mask) + ((*dst >> 1) & translucent_mask);
        } while (++src, ++dst, ++zdst, --data > 0);
      }
    }
    dst += line_skip;
    zdst += line_skip;
  } while (--height > 0);
#else
  line_skip *= 2;

  __asm {

		mov		esi, src
		mov		edi, dst
		xor		eax, eax
		mov		ebx, zdst
		xor		ecx, ecx
		mov		edx, pal

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:
		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL5

		xor		edx, edx
		xor		eax, eax
		mov		edx, pal
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		shr		eax, 1
		mov		dx, [edi]
		and		eax, [guiTranslucentMask]

		shr		edx, 1
		and		edx, [guiTranslucentMask]

		add		eax, edx
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, line_skip
		add		ebx, line_skip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

uint16_t *InitZBuffer(const uint32_t width, const uint32_t height) {
  return MALLOCNZ(uint16_t, width * height);
}

void ShutdownZBuffer(uint16_t *const pBuffer) { MemFree(pBuffer); }

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferMonoShadowClip

        Uses a bitmap an 8BPP template for blitting. Anywhere a 1 appears in the
bitmap, a shadow is blitted to the destination (a black pixel). Any other value
above zero is considered a forground color, and zero is background. If the
parameter for the background color is zero, transparency is used for the
background.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferMonoShadowClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                            HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                            uint16_t usIndex, SGPRect *clipregion,
                                            uint16_t usForeground, uint16_t usBackground,
                                            uint16_t usShadow) {
  uint32_t Unblitted;
  uint8_t *DestPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        if (usBackground == 0) {
          DestPtr += 2 * PxCount;
        } else {
          while (PxCount-- != 0) {
            *(uint16_t *)DestPtr = usBackground;
            DestPtr += 2;
          }
        }
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          switch (*SrcPtr++) {
            case 0:
              if (usBackground != 0) *(uint16_t *)DestPtr = usBackground;
              break;
            case 1:
              if (usShadow != 0) *(uint16_t *)DestPtr = usShadow;
              break;
            default:
              *(uint16_t *)DestPtr = usForeground;
              break;
          }
          DestPtr += 2;
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:
		xor		eax, eax
		mov		al, [esi]
		cmp		al, 1
		jne		BlitNTL3

        // write shadow pixel
		mov		ax, usShadow

        // only write if not zero
		cmp		ax, 0
		je		BlitNTL2

		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL3:
		or		al, al
		jz		BlitNTL4

        // write foreground pixel
		mov		ax, usForeground
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL4:
		cmp		usBackground, 0
		je		BlitNTL2

		mov		ax, usBackground
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:
		sub		LSCount, ecx

		mov		ax, usBackground
		or		ax, ax
		jz		BTrans2

		rep		stosw
		jmp		BlitDispatch

BTrans2:
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
        Blt16BPPTo16BPP

        Copies a rect of 16 bit data from a video buffer to a buffer position of
the brush in the data area, for later blitting. Used to copy background
information for mercs etc. to their unblit buffer, for later reblitting. Does
NOT clip.

**********************************************************************************************/
void Blt16BPPTo16BPP(uint16_t *pDest, uint32_t uiDestPitch, uint16_t *pSrc, uint32_t uiSrcPitch,
                     int32_t iDestXPos, int32_t iDestYPos, int32_t iSrcXPos, int32_t iSrcYPos,
                     uint32_t uiWidth, uint32_t uiHeight) {
#if 1  // XXX TODO
  uint32_t i;

  for (i = 0; i < uiHeight; i++) {
    memcpy((uint8_t *)pDest + uiDestPitch * (iDestYPos + i) + 2 * iDestXPos,
           (uint8_t *)pSrc + uiSrcPitch * (iSrcYPos + i) + 2 * iSrcXPos, uiWidth * 2);
  }
#else
  uint16_t *pSrcPtr, *pDestPtr;
  uint32_t uiLineSkipDest, uiLineSkipSrc;

  Assert(pDest != NULL);
  Assert(pSrc != NULL);

  pSrcPtr = (uint16_t *)((uint8_t *)pSrc + (iSrcYPos * uiSrcPitch) + (iSrcXPos * 2));
  pDestPtr = (uint16_t *)((uint8_t *)pDest + (iDestYPos * uiDestPitch) + (iDestXPos * 2));
  uiLineSkipDest = uiDestPitch - (uiWidth * 2);
  uiLineSkipSrc = uiSrcPitch - (uiWidth * 2);

  __asm {
	mov		esi, pSrcPtr
	mov		edi, pDestPtr
	mov		ebx, uiHeight
	cld

	mov		ecx, uiWidth
	test	ecx, 1
	jz		BlitDwords

BlitNewLine:

	mov		ecx, uiWidth
	shr		ecx, 1
	movsw

        // BlitNL2:

	rep		movsd

	add		edi, uiLineSkipDest
	add		esi, uiLineSkipSrc
	dec		ebx
	jnz		BlitNewLine

	jmp		BlitDone


BlitDwords:
	mov		ecx, uiWidth
	shr		ecx, 1
	rep		movsd

	add		edi, uiLineSkipDest
	add		esi, uiLineSkipSrc
	dec		ebx
	jnz		BlitDwords

BlitDone:

  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZPixelateObscured

        // OK LIKE NORMAL PIXELATE BUT ONLY PIXELATES STUFF BELOW Z level

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. The
Z-buffer is 16 bit, and must be the same dimensions (including Pitch) as the
destination.

        Blits every second pixel ("pixelates").

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransZPixelateObscured(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                    uint16_t *pZBuffer, uint16_t usZValue,
                                                    HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                    uint16_t usIndex) {
  uint32_t LineSkip;
  uint8_t *DestPtr, *ZPtr;
  uint32_t uiLineFlag;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));
  uiLineFlag = (iTempY & 1);

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        do {
          if (*(uint16_t *)ZPtr < usZValue) {
            *(uint16_t *)ZPtr = usZValue;
          } else {
            if (uiLineFlag != (((uintptr_t)DestPtr & 2) != 0)) continue;
          }
          *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
        } while (SrcPtr++, DestPtr += 2, ZPtr += 2, --data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
    uiLineFlag ^= 1;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx
		mov		edx, p16BPPPalette

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

        // TEST FOR Z FIRST!
		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL8

        // Write it NOW!
		jmp		BlitNTL7

BlitNTL8:

		test	uiLineFlag, 1
		jz		BlitNTL6

		test	edi, 2
		jz		BlitNTL5
		jmp		BlitNTL9

BlitNTL6:
		test	edi, 2
		jnz		BlitNTL5

BlitNTL7:

        // Write normal z value
		mov		ax, usZValue
		mov		[ebx], ax
        // jmp   BlitNTL10

BlitNTL9:

        // Write no z
        // mov		ax, 32767
        // mov		[ebx], ax

        // BlitNTL10:

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		xor		uiLineFlag, 1
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/* Blit an image into the destination buffer, using an ETRLE brush as a source
 * and a 16-bit buffer as a destination. As it is blitting, it checks the Z-
 * value of the ZBuffer, and if the pixel's Z level is below that of the current
 * pixel, it is written on, and the Z value is updated to the current value, for
 * any non-transparent pixels. The Z-buffer is 16 bit, and must be the same
 * dimensions (including Pitch) as the destination. */
void Blt8BPPDataTo16BPPBufferTransZ(uint16_t *const buf, uint32_t const uiDestPitchBYTES,
                                    uint16_t *const zbuf, uint16_t const zval,
                                    HVOBJECT const hSrcVObject, int32_t const iX, int32_t const iY,
                                    uint16_t const usIndex) {
  Assert(hSrcVObject);
  Assert(buf);

  // Get offsets from index into structure
  ETRLEObject const &e = hSrcVObject->SubregionProperties(usIndex);
  uint32_t height = e.usHeight;
  uint32_t const width = e.usWidth;

  // Add to start position of dest buffer
  int32_t const x = iX + e.sOffsetX;
  int32_t const y = iY + e.sOffsetY;

  // Validations
  CHECKV(x >= 0);
  CHECKV(y >= 0);

  uint8_t const *src = hSrcVObject->PixData(e);
  uint32_t const pitch = uiDestPitchBYTES / 2;
  uint16_t *dst = buf + pitch * y + x;
  uint16_t *zdst = zbuf + pitch * y + x;
  uint16_t const *const pal = hSrcVObject->CurrentShade();
  uint32_t line_skip = pitch - width;

#if 1  // XXX TODO
  for (;;) {
    uint8_t data = *src++;
    if (data == 0) {
      if (--height == 0) break;
      dst += line_skip;
      zdst += line_skip;
    } else if (data & 0x80) {
      data &= 0x7F;
      dst += data;
      zdst += data;
    } else {
      do {
        if (*zdst <= zval) {
          *zdst = zval;
          *dst = pal[*src];
        }
        ++src;
        ++dst;
        ++zdst;
      } while (--data != 0);
    }
  }
#else
  line_skip *= 2;

  __asm {

		mov		esi, src
		mov		edi, dst
		mov		edx, pal
		xor		eax, eax
		mov		ebx, zdst
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		xor		eax, eax

BlitNTL4:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jb		BlitNTL5

		mov		[ebx], ax

		xor		ah, ah
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		inc		edi
		inc		ebx
		inc		edi
		inc		ebx

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		height
		jz		BlitDone
		add		edi, line_skip
		add		ebx, line_skip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZNB

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on. The Z value is NOT updated by this version. The
Z-buffer is 16 bit, and	must be the same dimensions (including Pitch) as the
destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                      uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                      int32_t iX, int32_t iY, uint16_t usIndex) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        do {
          if (*(uint16_t *)ZPtr <= usZValue) {
            *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
        } while (--data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		xor		eax, eax

BlitNTL4:

		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL5

		xor		ah, ah
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		inc		edi
		inc		ebx
		inc		edi
		inc		ebx
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransShadow

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. If the
source pixel is 254, it is considered a shadow, and the destination buffer is
darkened rather than blitted on. The Z-buffer is 16 bit, and	must be the same
dimensions (including Pitch) as the destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransShadow(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                         HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                         uint16_t usIndex, const uint16_t *p16BPPPalette) {
  uint8_t *DestPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  (void)usHeight;
  UNIMPLEMENTED
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL6

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL5


BlitNTL6:
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransShadowZ

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. If the
source pixel is 254, it is considered a shadow, and the destination buffer is
darkened rather than blitted on. The Z-buffer is 16 bit, and	must be the same
dimensions (including Pitch) as the destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransShadowZ(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                          uint16_t *pZBuffer, uint16_t usZValue,
                                          HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                          uint16_t usIndex, const uint16_t *p16BPPPalette) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  (void)usHeight;
  UNIMPLEMENTED
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL5

		mov		ax, usZValue
		mov		[ebx], ax

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL6

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL5


BlitNTL6:
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransShadowZNB

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on. The Z value is NOT updated. If the source pixel is 254,
it is considered a shadow, and the destination buffer is darkened rather than
blitted on. The Z-buffer is 16 bit, and must be the same dimensions (including
Pitch) as the destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransShadowZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                            uint16_t *pZBuffer, uint16_t usZValue,
                                            HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                            uint16_t usIndex, const uint16_t *p16BPPPalette) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        do {
          uint8_t px = *SrcPtr++;

          if (px == 254) {
            if (*(uint16_t *)ZPtr < usZValue) {
              *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
            }
          } else {
            if (*(uint16_t *)ZPtr <= usZValue) {
              *(uint16_t *)DestPtr = p16BPPPalette[px];
            }
          }
          DestPtr += 2;
          ZPtr += 2;
        } while (--data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL5

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL6

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL5

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL5


BlitNTL6:
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransShadowZNBObscured

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on. The Z value is NOT updated. If the source pixel is 254,
it is considered a shadow, and the destination buffer is darkened rather than
blitted on. The Z-buffer is 16 bit, and must be the same dimensions (including
Pitch) as the destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransShadowZNBObscured(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                    uint16_t *pZBuffer, uint16_t usZValue,
                                                    HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                    uint16_t usIndex,
                                                    const uint16_t *p16BPPPalette) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  uint32_t uiLineFlag;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));
  uiLineFlag = (iTempY & 1);

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        do {
          uint8_t px = *SrcPtr++;

          if (px == 254) {
            if (*(uint16_t *)ZPtr < usZValue) {
              *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
            }
          } else {
            if (*(uint16_t *)ZPtr <= usZValue ||
                uiLineFlag == (((uintptr_t)DestPtr & 2) != 0))  // XXX ugly, can be done better by
                                                                // just examining every other pixel
            {
              *(uint16_t *)DestPtr = p16BPPPalette[px];
            }
          }
          DestPtr += 2;
          ZPtr += 2;
        } while (--data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
    uiLineFlag ^= 1;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:


		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL8

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL6

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL5

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL5


BlitNTL8:

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		je		BlitNTL5

		test	uiLineFlag, 1
		jz		BlitNTL9

		test	edi, 2
		jz		BlitNTL5

		jmp		BlitNTL6

BlitNTL9:
		test	edi, 2
		jnz		BlitNTL5


BlitNTL6:

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		xor		uiLineFlag, 1
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransShadowZClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. The
Z-buffer is 16 bit, and must be the same dimensions (including Pitch) as the
destination. Pixels with a value of 254 are shaded instead of blitted.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransShadowZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                              uint16_t *pZBuffer, uint16_t usZValue,
                                              HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                              uint16_t usIndex, SGPRect *clipregion,
                                              const uint16_t *p16BPPPalette) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  UNIMPLEMENTED
#else
  uint32_t Unblitted;
  int32_t LSCount;
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL2

		mov		ax, usZValue
		mov		[ebx], ax

		xor		eax, eax

		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL3

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL3:
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransShadowClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. The
Z-buffer is 16 bit, and must be the same dimensions (including Pitch) as the
destination. Pixels with a value of 254 are shaded instead of blitted.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransShadowClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                             HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                             uint16_t usIndex, SGPRect *clipregion,
                                             const uint16_t *p16BPPPalette) {
  uint8_t *DestPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  UNIMPLEMENTED
#else
  uint32_t Unblitted;
  int32_t LSCount;
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:
		xor		eax, eax

		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL3

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL3:
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransShadowZNBClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on. The Z-buffer is 16 bit, and	must be the same
dimensions (including Pitch) as the destination. Pixels with a value of	254 are
shaded instead of blitted. The Z buffer is NOT updated.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransShadowZNBClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                uint16_t *pZBuffer, uint16_t usZValue,
                                                HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                uint16_t usIndex, SGPRect *clipregion,
                                                const uint16_t *p16BPPPalette) {
  uint32_t Unblitted;
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          uint8_t px = *SrcPtr++;

          if (px == 254) {
            if (*(uint16_t *)ZPtr < usZValue) {
              *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
            }
          } else {
            if (*(uint16_t *)ZPtr <= usZValue) {
              *(uint16_t *)DestPtr = p16BPPPalette[px];
            }
          }
          DestPtr += 2;
          ZPtr += 2;
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL2

		xor		eax, eax

		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL3

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL2

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL3:
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransShadowZNBClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on. The Z-buffer is 16 bit, and	must be the same
dimensions (including Pitch) as the destination. Pixels with a value of	254 are
shaded instead of blitted. The Z buffer is NOT updated.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion,
    const uint16_t *p16BPPPalette) {
  uint32_t Unblitted, uiLineFlag;
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uiLineFlag = (iTempY + TopSkip) & 1;

  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          uint8_t px = *SrcPtr++;

          if (px == 254) {
            if (*(uint16_t *)ZPtr < usZValue) {
              *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
            }
          } else {
            if (*(uint16_t *)ZPtr <= usZValue ||
                uiLineFlag == (((uintptr_t)DestPtr & 2) != 0))  // XXX ugly, can be done better by
                                                                // just examining every other pixel
            {
              *(uint16_t *)DestPtr = p16BPPPalette[px];
            }
          }
          DestPtr += 2;
          ZPtr += 2;
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
    uiLineFlag ^= 1;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:

		xor		uiLineFlag, 1
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitPixellate

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL3

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL2

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL2

BlitPixellate:

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		je		BlitNTL2

		test	uiLineFlag, 1
		jz		BlitNTL9

		test	edi, 2
		jz		BlitNTL2
		jmp		BlitNTL3

BlitNTL9:
		test	edi, 2
		jnz		BlitNTL2


BlitNTL3:

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		xor		uiLineFlag, 1
		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferShadowZ

        Creates a shadow using a brush, but modifies the destination buffer only
if the current Z level is equal to higher than what's in the Z buffer at that
pixel location. It updates the Z buffer with the new Z level.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferShadowZ(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                     uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                     int32_t iX, int32_t iY, uint16_t usIndex) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        SrcPtr += data;
        do {
          if (*(uint16_t *)ZPtr < usZValue) {
            *(uint16_t *)ZPtr = usZValue;
            *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
          }
          DestPtr += 2;
          ZPtr += 2;
        } while (--data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET ShadeTable
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL5

		xor		eax, eax
		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax
		mov		ax, usZValue
		mov		[ebx], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferShadowZClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. The
Z-buffer is 16 bit, and must be the same dimensions (including Pitch) as the
destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferShadowZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                         uint16_t *pZBuffer, uint16_t usZValue,
                                         HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                         uint16_t usIndex, SGPRect *clipregion) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        SrcPtr += PxCount;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr < usZValue) {
            *(uint16_t *)ZPtr = usZValue;
            *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
          }
          DestPtr += 2;
          ZPtr += 2;
        } while (--PxCount > 0);
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  uint32_t Unblitted;
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET ShadeTable
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL2

		mov		ax, usZValue
		mov		[ebx], ax

		xor		eax, eax

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferShadowZNB

        Creates a shadow using a brush, but modifies the destination buffer only
if the current Z level is equal to higher than what's in the Z buffer at that
pixel location. It does NOT update the Z buffer with the new Z value.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferShadowZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                       uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                       int32_t iX, int32_t iY, uint16_t usIndex) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        do {
          if (*(uint16_t *)ZPtr < usZValue) {
            *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
          }
        } while (SrcPtr++, DestPtr += 2, ZPtr += 2, --data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET ShadeTable
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL5

		xor		eax, eax
		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferShadowZNBClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, the Z value is not updated,	for any non-transparent
pixels. The Z-buffer is 16 bit, and	must be the same dimensions (including
Pitch) as the destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferShadowZNBClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                           uint16_t *pZBuffer, uint16_t usZValue,
                                           HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                           uint16_t usIndex, SGPRect *clipregion) {
  uint32_t Unblitted;
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr < usZValue) {
            *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
          }
        } while (SrcPtr++, DestPtr += 2, ZPtr += 2, --PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET ShadeTable
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL2

		xor		eax, eax

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. The
Z-buffer is 16 bit, and must be the same dimensions (including Pitch) as the
destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                        uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                        int32_t iX, int32_t iY, uint16_t usIndex,
                                        SGPRect *clipregion) {
  uint32_t Unblitted;
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr <= usZValue) {
            *(uint16_t *)ZPtr = usZValue;
            *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL2

		mov		ax, usZValue
		mov		[ebx], ax

		xor		eax, eax

		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZNBClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on. The Z value is NOT updated in this version. The
Z-buffer is 16 bit, and must be the same dimensions (including Pitch) as the
destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransZNBClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                          uint16_t *pZBuffer, uint16_t usZValue,
                                          HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                          uint16_t usIndex, SGPRect *clipregion) {
  uint32_t Unblitted;
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr <= usZValue) {
            *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL2

		xor		eax, eax

		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/* Blit a subrect from a flat 8 bit surface to a 16-bit buffer. */
void Blt8BPPDataSubTo16BPPBuffer(uint16_t *const buf, uint32_t const uiDestPitchBYTES,
                                 SGPVSurface *const hSrcVSurface, uint8_t *const pSrcBuffer,
                                 uint32_t const src_pitch, int32_t const x, int32_t const y,
                                 SGPBox const *const rect) {
  Assert(hSrcVSurface);
  Assert(pSrcBuffer);
  Assert(buf);

  CHECKV(x >= 0);
  CHECKV(y >= 0);

  uint32_t const LeftSkip = rect->x;
  uint32_t const TopSkip = rect->y * src_pitch;
  uint32_t const BlitLength = rect->w;
  uint32_t BlitHeight = rect->h;
  uint32_t const src_skip = src_pitch - BlitLength;

  uint32_t const pitch = uiDestPitchBYTES / 2;
  uint8_t const *src = pSrcBuffer + TopSkip + LeftSkip;
  uint16_t *dst = buf + pitch * y + x;
  uint16_t const *const pal = hSrcVSurface->p16BPPPalette;
  uint32_t line_skip = pitch - BlitLength;

#if 1  // XXX TODO
  do {
    uint32_t w = BlitLength;
    do {
      *dst++ = pal[*src++];
    } while (--w != 0);
    src += src_skip;
    dst += line_skip;
  } while (--BlitHeight != 0);
#else
  line_skip *= 2;

  __asm {

		mov		esi, src  // pointer to current line start address in source
		mov		edi, dst  // pointer to current line start address in destination
		mov		ebx, BlitHeight  // line counter (goes top to bottom)
		mov		edx, pal  // conversion table

		sub		eax, eax
		sub		ecx, ecx

NewRow:
		mov		ecx, BlitLength  // pixels to blit count

BlitLoop:
		mov		al, [esi]
		xor		ah, ah

		shl		eax, 1  // make it into a word index
		mov		ax, [edx+eax]  // get 16-bit version of 8-bit pixel
		mov		[edi], ax  // store it in destination buffer

		inc		edi
		inc		esi
		inc		edi
		dec		ecx
		jnz		BlitLoop

		add		esi, src_skip  // move line pointers down one line
		add		edi, line_skip

		dec		ebx  // check line counter
		jnz		NewRow  // done blitting, exit

    // DoneBlit: // finished blit
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBuffer

        Blits from a flat surface to a 16-bit buffer.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBuffer(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                              SGPVSurface *hSrcVSurface, uint8_t *pSrcBuffer, int32_t iX,
                              int32_t iY) {
  uint32_t LineSkip;
  int32_t iTempX, iTempY;

  // Assertions
  Assert(hSrcVSurface != NULL);
  Assert(pSrcBuffer != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  uint32_t const usWidth = hSrcVSurface->Width();
  uint32_t const usHeight = hSrcVSurface->Height();

  // Add to start position of dest buffer
  iTempX = iX;
  iTempY = iY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t *SrcPtr = pSrcBuffer;
  uint16_t *DestPtr = pBuffer + uiDestPitchBYTES / 2 * iTempY + iTempX;
  uint16_t *p16BPPPalette = hSrcVSurface->p16BPPPalette;
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  for (size_t h = usHeight; h != 0; --h) {
    for (size_t w = 0; w != usWidth; ++w) {
      DestPtr[w] = p16BPPPalette[SrcPtr[w]];
    }

    SrcPtr += usWidth;
    DestPtr += uiDestPitchBYTES / 2;
  }
#else
  uint32_t rows;
  __asm {

		mov		esi, SrcPtr  // pointer to current line start address in source
		mov		edi, DestPtr  // pointer to current line start address in destination
		mov		ecx, usHeight  // line counter (goes top to bottom)
		mov		rows, ecx
		mov		edx, p16BPPPalette

		sub		eax, eax
		sub		ecx, ecx

		mov		ebx, usWidth  // column counter (goes right to left)
		dec		ebx

ReadMask:
		test	usWidth, 1
		jz		BlitWord

		xor		eax, eax  // clear out the top 24 bits
		mov		al, [esi+ebx]

		shl		eax, 1  // make it into a word index
		mov		ax, [edx+eax]  // get 16-bit version of 8-bit pixel
		mov		[edi+ebx*2], ax  // store it in destination buffer

		dec		ebx
		js		DoneRow

BlitWord:

		test	usWidth, 2
		jz		SetupDwords


		mov		ax, [esi+ebx-1]
		mov		cl, ah
		sub		ah, ah
		and		ecx, 0ffH
		shl		eax, 1
		shl		ecx, 1
		mov		ax, [edx+eax]
		mov		cx, [edx+ecx]
		shl		ecx, 16
		mov		cx, ax
		mov		[edi+ebx*2-2], ecx

		sub		ebx, 2
		js		DoneRow

SetupDwords:


BlitDwords:

		mov		ax, [esi+ebx-1]
		mov		cl, ah
		sub		ah, ah
		and		ecx, 0ffH
		shl		eax, 1
		shl		ecx, 1
		mov		ax, [edx+eax]
		mov		cx, [edx+ecx]
		shl		ecx, 16
		mov		cx, ax
		mov		[edi+ebx*2-2], ecx

		mov		ax, [esi+ebx-3]
		mov		cl, ah
		sub		ah, ah
		and		ecx, 0ffH
		shl		eax, 1
		shl		ecx, 1
		mov		ax, [edx+eax]
		mov		cx, [edx+ecx]
		shl		ecx, 16
		mov		cx, ax
		mov		[edi+ebx*2-6], ecx

		sub		ebx, 4  // decrement column counter
		jns		BlitDwords  // loop until one line is done

DoneRow:
		dec		rows  // check line counter
		jz		DoneBlit  // done blitting, exit

		add		esi, usWidth  // move line pointers down one line
		add		edi, uiDestPitchBYTES
		mov		ebx, usWidth  // column counter (goes right to left)
		dec		ebx
		jmp		ReadMask

DoneBlit:  // finished blit
  }
#endif
}

/* Blit from a flat surface to a 16-bit buffer, dividing the source image into
 * exactly half the size, optionally from a sub-region.
 * - Source rect is in source units.
 * - In order to make sure the same pixels are skipped, always align the top and
 *   left coordinates to the same factor of two.
 * - A rect specifying an odd number of pixels will divide out to an even number
 *   of pixels blitted to the destination. */
void Blt8BPPDataTo16BPPBufferHalf(uint16_t *const dst_buf, uint32_t const uiDestPitchBYTES,
                                  SGPVSurface *const src_surface, uint8_t const *const src_buf,
                                  uint32_t const src_pitch, int32_t const x, int32_t const y,
                                  SGPBox const *const rect) {
  Assert(src_surface);
  Assert(src_buf);
  Assert(dst_buf);

  CHECKV(x >= 0);
  CHECKV(y >= 0);

  uint8_t const *src = src_buf;
  uint32_t width;
  uint32_t height;
  if (rect) {
    width = rect->w;
    height = rect->h;
    CHECKV(0 < width && width <= src_surface->Width());
    CHECKV(0 < height && height <= src_surface->Height());

    src += src_pitch * rect->y + rect->x;
  } else {
    width = src_surface->Width();
    height = src_surface->Height();
  }

  uint16_t *dst = dst_buf + uiDestPitchBYTES / 2 * y + x;
  uint32_t const src_skip = (src_pitch - width / 2) * 2;
  uint32_t const dst_skip = uiDestPitchBYTES / 2 - width / 2;
  uint16_t const *const pal = src_surface->p16BPPPalette;

#if 1  // XXX TODO
  height /= 2;
  do {
    uint32_t w = width / 2;
    do {
      *dst++ = pal[*src];
      src += 2;
    } while (--w > 0);
    src += src_skip;
    dst += dst_skip;
  } while (--height > 0);
#else
  __asm {

		mov		esi, src  // pointer to current line start address in source
		mov		edi, dst  // pointer to current line start address in destination
		mov		ebx, height  // line counter (goes top to bottom)
		shr		ebx, 1  // half the rows
		mov		edx, pal

		xor		eax, eax

BlitSetup:
		mov		ecx, width
		shr		ecx, 1  // divide the width by 2

ReadMask:
		mov		al, [esi]
		xor		ah, ah
		inc		esi  // skip one source byte
		inc		esi

		shl		eax, 1  // make it into a word index
		mov		ax, [edx+eax]  // get 16-bit version of 8-bit pixel
		mov		[edi], ax  // store it in destination buffer
		inc		edi  // next pixel
		inc		edi

		dec		ecx
		jnz		ReadMask

                    // DoneRow:

		add		esi, src_skip  // move source pointer down one line
		add		edi, dst_skip

		dec		ebx  // check line counter
		jnz		BlitSetup  // done blitting, exit

    // DoneBlit: // finished blit
  }
#endif
}

void SetClippingRect(SGPRect *clip) {
  Assert(clip != NULL);
  Assert(clip->iLeft < clip->iRight);
  Assert(clip->iTop < clip->iBottom);
  ClippingRect = *clip;
}

void GetClippingRect(SGPRect *clip) {
  Assert(clip != NULL);
  *clip = ClippingRect;
}

/**********************************************************************************************
        Blt16BPPBufferPixelateRectWithColor

                Given an 8x8 pattern and a color, pixelates an area by
repeatedly "applying the color" to pixels whereever there is a non-zero value in
the pattern.

                KM:  Added Nov. 23, 1998
                This is all the code that I moved from
Blt16BPPBufferPixelateRect(). This function now takes a color field (which
previously was always black.  The 3rd assembler line in this function:

                                mov		ax, usColor
// color of pixel

                used to be:

                                xor   eax, eax
// color of pixel (black or 0)

          This was the only internal modification I made other than adding the
usColor argument.

*********************************************************************************************/
static void Blt16BPPBufferPixelateRectWithColor(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                SGPRect *area, const uint8_t Pattern[8][8],
                                                uint16_t usColor) {
  int32_t width, height;
  uint32_t LineSkip;
  uint16_t *DestPtr;
  int32_t iLeft, iTop, iRight, iBottom;

  // Assertions
  Assert(pBuffer != NULL);
  Assert(Pattern != NULL);

  iLeft = std::max(ClippingRect.iLeft, area->iLeft);
  iTop = std::max(ClippingRect.iTop, area->iTop);
  iRight = std::min(ClippingRect.iRight - 1, area->iRight);
  iBottom = std::min(ClippingRect.iBottom - 1, area->iBottom);

  DestPtr = (pBuffer + (iTop * (uiDestPitchBYTES / 2)) + iLeft);
  width = iRight - iLeft + 1;
  height = iBottom - iTop + 1;
  LineSkip = (uiDestPitchBYTES - (width * 2));

  CHECKV(width >= 1);
  CHECKV(height >= 1);

#if 1  // XXX TODO
  uint32_t row = 0;
  do {
    uint32_t col = 0;
    uint32_t w = width;

    do {
      if (Pattern[row][col] != 0) *DestPtr = usColor;
      DestPtr++;
      col = (col + 1) % 8;
    } while (--w > 0);
    DestPtr += LineSkip / 2;
    row = (row + 1) % 8;
  } while (--height > 0);
#else
  __asm {
		mov		esi, Pattern  // Pointer to pixel pattern
		mov		edi, DestPtr  // Pointer to top left of rect area
		mov		ax, usColor  // color of pixel
		xor		ebx, ebx  // pattern column index
		xor		edx, edx  // pattern row index


BlitNewLine:
		mov		ecx, width

BlitLine:
		cmp	[esi+ebx], 0
		je	BlitLine2

		mov		[edi], ax

BlitLine2:
		add		edi, 2
		inc		ebx
		and		ebx, 07H
		or		ebx, edx
		dec		ecx
		jnz		BlitLine

		add		edi, LineSkip
		xor		ebx, ebx
		add		edx, 08H
		and		edx, 38H
		dec		height
		jnz		BlitNewLine
  }
#endif
}

// Uses black hatch color
void Blt16BPPBufferHatchRect(uint16_t *pBuffer, uint32_t uiDestPitchBYTES, SGPRect *area) {
  const uint8_t Pattern[8][8] = {{1, 0, 1, 0, 1, 0, 1, 0}, {0, 1, 0, 1, 0, 1, 0, 1},
                                 {1, 0, 1, 0, 1, 0, 1, 0}, {0, 1, 0, 1, 0, 1, 0, 1},
                                 {1, 0, 1, 0, 1, 0, 1, 0}, {0, 1, 0, 1, 0, 1, 0, 1},
                                 {1, 0, 1, 0, 1, 0, 1, 0}, {0, 1, 0, 1, 0, 1, 0, 1}};
  Blt16BPPBufferPixelateRectWithColor(pBuffer, uiDestPitchBYTES, area, Pattern, 0);
}

void Blt16BPPBufferLooseHatchRectWithColor(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                           SGPRect *area, uint16_t usColor) {
  const uint8_t Pattern[8][8] = {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
                                 {0, 0, 1, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
                                 {1, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
                                 {0, 0, 1, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 0, 0}};
  Blt16BPPBufferPixelateRectWithColor(pBuffer, uiDestPitchBYTES, area, Pattern, usColor);
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferShadow

        Modifies the destination buffer. Darkens the destination pixels by 25%,
using the source image as a mask. Any Non-zero index pixels are used to darken
destination pixels.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferShadow(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                    HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                    uint16_t usIndex) {
  uint8_t *DestPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
      } else {
        SrcPtr += data;
        do {
          *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
          DestPtr += 2;
        } while (--data > 0);
      }
    }
    DestPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, usHeight
		xor		ecx, ecx
		mov		edx, OFFSET ShadeTable

BlitDispatch:


		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		xor		eax, eax

		add		esi, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		edi, 8
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		ebx
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/* Blit an image into the destination buffer, using an ETRLE brush as a source
 * and a 16-bit buffer as a destination. */
void Blt8BPPDataTo16BPPBufferTransparent(uint16_t *const buf, uint32_t const uiDestPitchBYTES,
                                         SGPVObject const *const hSrcVObject, int32_t const iX,
                                         int32_t const iY, uint16_t const usIndex) {
  Assert(hSrcVObject);
  Assert(buf);

  // Get offsets from index into structure
  ETRLEObject const &e = hSrcVObject->SubregionProperties(usIndex);
  uint32_t height = e.usHeight;
  uint32_t const width = e.usWidth;

  // Add to start position of dest buffer
  int32_t const x = iX + e.sOffsetX;
  int32_t const y = iY + e.sOffsetY;

  CHECKV(x >= 0);
  CHECKV(y >= 0);

  uint32_t const pitch = uiDestPitchBYTES / 2;
  uint8_t const *src = hSrcVObject->PixData(e);
  uint16_t *dst = buf + pitch * y + x;
  uint16_t const *const pal = hSrcVObject->CurrentShade();
  uint32_t line_skip = pitch - width;

#if 1  // XXX TODO
  for (;;) {
    uint8_t data = *src++;
    if (data == 0) {
      if (--height == 0) break;
      dst += line_skip;
    } else if (data & 0x80) {  // Transparent
      dst += data & 0x7F;
    } else {
      do {
        *dst++ = pal[*src++];
      } while (--data != 0);
    }
  }
#else
  line_skip *= 2;

  __asm {

		mov		esi, src
		mov		edi, dst
		mov		edx, pal
		xor		eax, eax
		xor		ebx, ebx
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		mov		bl, [esi+1]
		mov		ax, [edx+ebx*2]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

		xor		ebx, ebx

BlitNTL4:

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		mov		bl, [esi+1]
		mov		ax, [edx+ebx*2]
		mov		[edi+2], ax

		mov		bl, [esi+2]
		mov		ax, [edx+ebx*2]
		mov		[edi+4], ax

		mov		bl, [esi+3]
		mov		ax, [edx+ebx*2]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		height
		jz		BlitDone
		add		edi, line_skip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransparentClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. Clips the brush.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransparentClip(uint16_t *const pBuffer,
                                             const uint32_t uiDestPitchBYTES,
                                             const SGPVObject *const hSrcVObject, const int32_t iX,
                                             const int32_t iY, const uint16_t usIndex,
                                             const SGPRect *const clipregion) {
  uint32_t Unblitted;
  uint8_t *DestPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uint32_t LSCount;
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr++];
          DestPtr += 2;
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, TopSkip
		xor		ecx, ecx

		or		ebx, ebx  // check for nothing clipped on top
		jz		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		ebx
		jnz		TopSkipLoop




LeftSkipSetup:

		mov		Unblitted, 0
		mov		ebx, LeftSkip  // check for nothing clipped on the left
		or		ebx, ebx
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ebx
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, ebx  // skip partial run, jump into normal loop for rest
		sub		ecx, ebx
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		ebx, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, ebx
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, ebx  // skip partial run, jump into normal loop for rest
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		ebx, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop




BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		ebx, BlitLength
		mov		Unblitted, 0

BlitDispatch:

		or		ebx, ebx  // Check to see if we're done blitting
		jz		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, ebx
		jbe		BNTrans1

		sub		ecx, ebx
		mov		Unblitted, ecx
		mov		ecx, ebx

BNTrans1:
		sub		ebx, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		xor		eax, eax
		mov		al, [esi+1]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitLineEnd

BlitNTL4:

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		xor		eax, eax
		mov		al, [esi+1]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		xor		eax, eax
		mov		al, [esi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		xor		eax, eax
		mov		al, [esi+3]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, ebx
		jbe		BTrans1

		mov		ecx, ebx

BTrans1:

		sub		ebx, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 BltIsClipped

        Determines whether a given blit will need clipping or not. Returns
TRUE/FALSE.

**********************************************************************************************/
BOOLEAN BltIsClipped(const SGPVObject *const hSrcVObject, const int32_t iX, const int32_t iY,
                     const uint16_t usIndex, const SGPRect *const clipregion) {
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  if (std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth)) return (TRUE);

  if (std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth))
    return (TRUE);

  if (std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight)) return (TRUE);

  if (std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight))
    return (TRUE);

  return (FALSE);
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferShadowClip

        Modifies the destination buffer. Darkens the destination pixels by 25%,
using the source image as a mask. Any Non-zero index pixels are used to darken
destination pixels. Blitter clips brush if it doesn't fit on the viewport.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferShadowClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                        HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                        uint16_t usIndex, SGPRect *clipregion) {
  uint8_t *DestPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  uint32_t LSCount;
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        SrcPtr += PxCount;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;

        do {
          *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
          DestPtr += 2;
        } while (--PxCount > 0);
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  uint32_t Unblitted;
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET ShadeTable
		xor		eax, eax
		mov		ebx, TopSkip
		xor		ecx, ecx

		or		ebx, ebx  // check for nothing clipped on top
		jz		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		ebx
		jnz		TopSkipLoop




LeftSkipSetup:

		mov		Unblitted, 0
		mov		ebx, LeftSkip  // check for nothing clipped on the left
		or		ebx, ebx
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ebx
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, ebx  // skip partial run, jump into normal loop for rest
		sub		ecx, ebx
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		ebx, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, ebx
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, ebx  // skip partial run, jump into normal loop for rest
		mov		ebx, BlitLength
		jmp		BlitTransparent


LSTrans1:
		sub		ebx, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop




BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		ebx, BlitLength
		mov		Unblitted, 0

BlitDispatch:

		or		ebx, ebx  // Check to see if we're done blitting
		jz		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:

		cmp		ecx, ebx
		jbe		BNTrans1

		sub		ecx, ebx
		mov		Unblitted, ecx
		mov		ecx, ebx

BNTrans1:
		sub		ebx, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitLineEnd

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
		cmp		ecx, ebx
		jbe		BTrans1

		mov		ecx, ebx

BTrans1:

		sub		ebx, ecx
         //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

void Blt16BPPBufferFilterRect(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                              const uint16_t *filter_table, SGPRect *area) {
  int32_t width, height;
  uint32_t LineSkip;
  uint16_t *DestPtr;

  // Assertions
  Assert(pBuffer != NULL);

  // Clipping
  if (area->iLeft < ClippingRect.iLeft) area->iLeft = ClippingRect.iLeft;
  if (area->iTop < ClippingRect.iTop) area->iTop = ClippingRect.iTop;
  if (area->iRight >= ClippingRect.iRight) area->iRight = ClippingRect.iRight - 1;
  if (area->iBottom >= ClippingRect.iBottom) area->iBottom = ClippingRect.iBottom - 1;
  // CHECKF(area->iLeft >= ClippingRect.iLeft );
  // CHECKF(area->iTop >= ClippingRect.iTop );
  // CHECKF(area->iRight <= ClippingRect.iRight );
  // CHECKF(area->iBottom <= ClippingRect.iBottom );

  DestPtr = (pBuffer + (area->iTop * (uiDestPitchBYTES / 2)) + area->iLeft);
  width = area->iRight - area->iLeft + 1;
  height = area->iBottom - area->iTop + 1;
  LineSkip = (uiDestPitchBYTES - (width * 2));

  CHECKV(width >= 1);
  CHECKV(height >= 1);

#if 1  // XXX TODO
  do {
    uint32_t w = width;

    do {
      *DestPtr = filter_table[*DestPtr];
      DestPtr++;
    } while (--w > 0);
    DestPtr = (uint16_t *)((uint8_t *)DestPtr + LineSkip);
  } while (--height > 0);
#else
  __asm {
		mov		esi, filter_table
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, LineSkip
		mov		edx, height

BlitNewLine:
		mov		ecx, width

BlitLine:
		mov		ax, [edi]
		mov		ax, [esi+eax*2]
		mov		[edi], ax
		add		edi, 2
		dec		ecx
		jnz		BlitLine

		add		edi, ebx
		dec		edx
		jnz		BlitNewLine
  }
#endif
}

/**********************************************************************************************
 BltIsClippedOrOffScreen

        Determines whether a given blit will need clipping or not. Returns
TRUE/FALSE.

**********************************************************************************************/
char BltIsClippedOrOffScreen(HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex,
                             SGPRect *clipregion) {
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  int32_t gLeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  int32_t gTopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  int32_t gRightSkip =
      std::min(std::max(ClipX2, iTempX + (int32_t)usWidth) - ClipX2, (int32_t)usWidth);
  int32_t gBottomSkip =
      std::min(std::max(ClipY2, iTempY + (int32_t)usHeight) - ClipY2, (int32_t)usHeight);

  // check if whole thing is clipped
  if ((gLeftSkip >= (int32_t)usWidth) || (gRightSkip >= (int32_t)usWidth)) return (-1);

  // check if whole thing is clipped
  if ((gTopSkip >= (int32_t)usHeight) || (gBottomSkip >= (int32_t)usHeight)) return (-1);

  if (gLeftSkip) return (TRUE);

  if (gRightSkip) return (TRUE);

  if (gTopSkip) return (TRUE);

  if (gBottomSkip) return (TRUE);

  return (FALSE);
}

// ATE New blitter for rendering a differrent color for value 254. Can be
// transparent if outline is SGP_TRANSPARENT
void Blt8BPPDataTo16BPPBufferOutline(uint16_t *const buf, uint32_t const uiDestPitchBYTES,
                                     SGPVObject const *const hSrcVObject, int32_t const iX,
                                     int32_t const iY, uint16_t const usIndex,
                                     int16_t const outline) {
  Assert(hSrcVObject);
  Assert(buf);

  // Get offsets from index into structure
  ETRLEObject const &e = hSrcVObject->SubregionProperties(usIndex);
  uint32_t height = e.usHeight;
  uint32_t const width = e.usWidth;

  // Add to start position of dest buffer
  int32_t const x = iX + e.sOffsetX;
  int32_t const y = iY + e.sOffsetY;

  CHECKV(x >= 0);
  CHECKV(y >= 0);

  uint8_t const *src = hSrcVObject->PixData(e);
  uint32_t const pitch = uiDestPitchBYTES / 2;
  uint16_t *dst = buf + pitch * y + x;
  uint32_t line_skip = pitch - width;
  uint16_t const *const pal = hSrcVObject->CurrentShade();

#if 1  // XXX TODO
  for (;;) {
    uint8_t data = *src++;
    if (data == 0) {
      if (--height == 0) break;
      dst += line_skip;
    } else if (data & 0x80) {
      dst += data & 0x7F;
    } else {
      do {
        uint8_t const px = *src++;
        if (px != 254) {
          *dst = pal[px];
        } else if (outline != SGP_TRANSPARENT) {
          *dst = outline;
        }
        ++dst;
      } while (--data != 0);
    }
  }
#else
  line_skip *= 2;

  __asm {

		mov		esi, src
		mov		edi, dst
		mov		edx, pal
		xor		eax, eax
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL6

        //		DO OUTLINE
        //		ONLY IF WE WANT IT!
		mov		al, fDoOutline;
		cmp		al,	1
		jne		BlitNTL5

		mov		ax, s16BPPColor
		mov		[edi], ax
		jmp		BlitNTL5


BlitNTL6:
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		height
		jz		BlitDone
		add		edi, line_skip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

// ATE New blitter for rendering a differrent color for value 254. Can be
// transparent if s16BPPColor is SGP_TRANSPARENT
void Blt8BPPDataTo16BPPBufferOutlineClip(uint16_t *const pBuffer, const uint32_t uiDestPitchBYTES,
                                         const SGPVObject *const hSrcVObject, const int32_t iX,
                                         const int32_t iY, const uint16_t usIndex,
                                         const int16_t s16BPPColor,
                                         const SGPRect *const clipregion) {
  uint32_t Unblitted;
  uint8_t *DestPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          uint32_t src = *SrcPtr++;

          if (src != 254) {
            *(uint16_t *)DestPtr = p16BPPPalette[src];
          } else if (s16BPPColor != SGP_TRANSPARENT) {
            *(uint16_t *)DestPtr = s16BPPColor;
          }
          DestPtr += 2;
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:
		xor		eax, eax

		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL3

        //		DO OUTLINE
        //		ONLY IF WE WANT IT!
		mov		al, fDoOutline;
		cmp		al,	1
		jne		BlitNTL2

		mov		ax, s16BPPColor
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL3:
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

void Blt8BPPDataTo16BPPBufferOutlineZClip(uint16_t *const pBuffer, const uint32_t uiDestPitchBYTES,
                                          uint16_t *const pZBuffer, const uint16_t usZValue,
                                          const HVOBJECT hSrcVObject, const int32_t iX,
                                          const int32_t iY, const uint16_t usIndex,
                                          const int16_t s16BPPColor,
                                          const SGPRect *const clipregion) {
  uint32_t Unblitted;
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);

  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr <= usZValue) {
            uint32_t src = *SrcPtr;

            if (src != 254) {
              *(uint16_t *)ZPtr = usZValue;
              *(uint16_t *)DestPtr = p16BPPPalette[src];
            } else {
              *(uint16_t *)DestPtr = s16BPPColor;
            }
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit an end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jb		BlitNTL2

        // CHECK FOR OUTLINE...
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL3

        //		DO OUTLINE
        //		ONLY IF WE WANT IT!
		mov		al, fDoOutline;
		cmp		al,	1
		jne		BlitNTL2

		mov		ax, s16BPPColor
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL3:

        // Write to z-buffer
		mov		[ebx], ax

		xor		eax, eax

		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		inc		ebx
		inc		ebx
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

void Blt8BPPDataTo16BPPBufferOutlineZPixelateObscuredClip(
    uint16_t *const pBuffer, const uint32_t uiDestPitchBYTES, uint16_t *const pZBuffer,
    const uint16_t usZValue, const HVOBJECT hSrcVObject, const int32_t iX, const int32_t iY,
    const uint16_t usIndex, const int16_t s16BPPColor, const SGPRect *const clipregion) {
  uint32_t Unblitted;
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;
  uint32_t uiLineFlag;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);

  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  uiLineFlag = (iTempY & 1);

#if 1  // XXX TODO
  uiLineFlag = (iTempY + TopSkip) & 1;

  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr <= usZValue) {
            *(uint16_t *)ZPtr = usZValue;
          } else {
            // XXX original code updates Z value in one of two cases on this
            // path, seems wrong
            if (uiLineFlag != (((uintptr_t)DestPtr & 2) != 0)) continue;
          }

          uint8_t px = *SrcPtr;
          if (px == 254) {
            *(uint16_t *)DestPtr = s16BPPColor;
          } else {
            // XXX original code writes garbage (lower 8 bit are the colour
            // index) into the Z buffer at this point
            *(uint16_t *)DestPtr = p16BPPPalette[px];
          }
        } while (SrcPtr++, DestPtr += 2, ZPtr += 2, --PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
    uiLineFlag ^= 1;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jb		BlitNTL8

        // Write it now!
		jmp BlitNTL7

BlitNTL8:

		test	uiLineFlag, 1
		jz		BlitNTL6

		test	edi, 2
		jz		BlitNTL2
		jmp		BlitNTL9


BlitNTL6:

		test	edi, 2
		jnz		BlitNTL2

BlitNTL7:

		mov		[ebx], ax

BlitNTL9:

        // CHECK FOR OUTLINE...
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL3

        //		DO OUTLINE
        //		ONLY IF WE WANT IT!
		mov		al, fDoOutline;
		cmp		al,	1
		jne		BlitNTL2

		mov		ax, s16BPPColor
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL3:

        // Write to z-buffer
		mov		[ebx], ax

		xor		eax, eax

		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		inc		ebx
		inc		ebx
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		xor		uiLineFlag, 1

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

void Blt8BPPDataTo16BPPBufferOutlineShadow(uint16_t *const pBuffer, const uint32_t uiDestPitchBYTES,
                                           const SGPVObject *const hSrcVObject, const int32_t iX,
                                           const int32_t iY, const uint16_t usIndex) {
  uint8_t *DestPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += data * 2;
      } else {
        do {
          if (*SrcPtr++ != 254) {
            *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
          }
          DestPtr += 2;
        } while (--data > 0);
      }
    }
    DestPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		je		BlitNTL5

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax


BlitNTL5:
		inc		esi
		add		edi, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

void Blt8BPPDataTo16BPPBufferOutlineShadowClip(
    uint16_t *const pBuffer, const uint32_t uiDestPitchBYTES, const SGPVObject *const hSrcVObject,
    const int32_t iX, const int32_t iY, const uint16_t usIndex, const SGPRect *const clipregion) {
  uint8_t *DestPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  UNIMPLEMENTED
#else
  uint32_t Unblitted;
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET ShadeTable
		xor		eax, eax
		mov		ebx, TopSkip
		xor		ecx, ecx

		or		ebx, ebx  // check for nothing clipped on top
		jz		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

        // Check for outline as well
		mov		cl, [esi]
		cmp		cl, 254
		je		TopSkipLoop
        //

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		ebx
		jnz		TopSkipLoop




LeftSkipSetup:

		mov		Unblitted, 0
		mov		ebx, LeftSkip  // check for nothing clipped on the left
		or		ebx, ebx
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ebx
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, ebx  // skip partial run, jump into normal loop for rest
		sub		ecx, ebx
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		ebx, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, ebx
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, ebx  // skip partial run, jump into normal loop for rest
		mov		ebx, BlitLength
		jmp		BlitTransparent


LSTrans1:
		sub		ebx, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop




BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		ebx, BlitLength
		mov		Unblitted, 0

BlitDispatch:

		or		ebx, ebx  // Check to see if we're done blitting
		jz		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:

		cmp		ecx, ebx
		jbe		BNTrans1

		sub		ecx, ebx
		mov		Unblitted, ecx
		mov		ecx, ebx

BNTrans1:
		sub		ebx, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitLineEnd

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
		cmp		ecx, ebx
		jbe		BTrans1

		mov		ecx, ebx

BTrans1:

		sub		ebx, ecx
         //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

void Blt8BPPDataTo16BPPBufferOutlineZ(uint16_t *const pBuffer, const uint32_t uiDestPitchBYTES,
                                      uint16_t *const pZBuffer, const uint16_t usZValue,
                                      const HVOBJECT hSrcVObject, const int32_t iX,
                                      const int32_t iY, const uint16_t usIndex,
                                      const int16_t s16BPPColor) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        do {
          if (*(uint16_t *)ZPtr <= usZValue) {
            uint8_t px = *SrcPtr;

            if (px == 254) {
              *(uint16_t *)DestPtr = s16BPPColor;
            } else {
              *(uint16_t *)ZPtr = usZValue;  // XXX TODO original code writes garbage into the Z
                                             // buffer, but comment says don't write at all
              *(uint16_t *)DestPtr = p16BPPPalette[px];
            }
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
        } while (--data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		xor		eax, eax

BlitNTL4:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jb		BlitNTL5

        // CHECK FOR OUTLINE, BLIT DIFFERENTLY IF WE WANT IT TO!
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL6

        //		DO OUTLINE
        //		ONLY IF WE WANT IT!
		mov		al, fDoOutline;
		cmp		al,	1
		jne		BlitNTL5

		mov		ax, s16BPPColor
		mov		[edi], ax
		jmp		BlitNTL5

BlitNTL6:

        // Donot write to z-buffer
		mov		[ebx], ax

		xor		ah, ah
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		inc		edi
		inc		ebx
		inc		edi
		inc		ebx

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

void Blt8BPPDataTo16BPPBufferOutlineZPixelateObscured(
    uint16_t *const pBuffer, const uint32_t uiDestPitchBYTES, uint16_t *const pZBuffer,
    const uint16_t usZValue, const HVOBJECT hSrcVObject, const int32_t iX, const int32_t iY,
    const uint16_t usIndex, const int16_t s16BPPColor) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  uint32_t uiLineFlag;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));
  uiLineFlag = (iTempY & 1);

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        do {
          if (*(uint16_t *)ZPtr < usZValue) {
            *(uint16_t *)ZPtr = usZValue;
          } else {
            // XXX original code updates Z value in one of two cases on this
            // path, seems wrong
            if (uiLineFlag != (((uintptr_t)DestPtr & 2) != 0)) continue;
          }

          uint8_t px = *SrcPtr;
          if (px == 254) {
            *(uint16_t *)DestPtr = s16BPPColor;
          } else {
            *(uint16_t *)DestPtr = p16BPPPalette[px];
          }
        } while (SrcPtr++, DestPtr += 2, ZPtr += 2, --data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
    uiLineFlag ^= 1;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		xor		eax, eax

BlitNTL4:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jbe		BlitNTL8

        // Write it now!
		jmp BlitNTL7

BlitNTL8:

		test	uiLineFlag, 1
		jz		BlitNTL6

		test	edi, 2
		jz		BlitNTL5
		jmp		BlitNTL9


BlitNTL6:

		test	edi, 2
		jnz		BlitNTL5

BlitNTL7:

		mov		[ebx], ax

BlitNTL9:

        // CHECK FOR OUTLINE, BLIT DIFFERENTLY IF WE WANT IT TO!
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL12

        //		DO OUTLINE
        //		ONLY IF WE WANT IT!
		mov		al, fDoOutline;
		cmp		al,	1
		jne		BlitNTL5

		mov		ax, s16BPPColor
		mov		[edi], ax
		jmp		BlitNTL5

BlitNTL12:

		xor		ah, ah
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		inc		edi
		inc		ebx
		inc		edi
		inc		ebx

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		xor		uiLineFlag, 1
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

// This is the same as above, but DONOT WRITE to Z!
void Blt8BPPDataTo16BPPBufferOutlineZNB(uint16_t *const pBuffer, const uint32_t uiDestPitchBYTES,
                                        uint16_t *const pZBuffer, const uint16_t usZValue,
                                        const HVOBJECT hSrcVObject, const int32_t iX,
                                        const int32_t iY, const uint16_t usIndex) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        DestPtr += 2 * data;
        ZPtr += 2 * data;
      } else {
        do {
          if (*(uint16_t *)ZPtr < usZValue) {
            uint8_t px = *SrcPtr;
            if (px != 254) {
              *(uint16_t *)DestPtr = p16BPPPalette[px];
            }
          }
        } while (SrcPtr++, DestPtr += 2, ZPtr += 2, --data > 0);
      }
    }
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		xor		eax, eax

BlitNTL4:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jb		BlitNTL5

        // CHECK FOR OUTLINE, BLIT DIFFERENTLY IF WE WANT IT TO!
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL6

        //		DO OUTLINE
        //		ONLY IF WE WANT IT!
		mov		al, fDoOutline;
		cmp		al,	1
		jne		BlitNTL5

		mov		ax, s16BPPColor
		mov		[edi], ax
		jmp		BlitNTL5

BlitNTL6:

        // Donot write to z-buffer
        // mov		[ebx], ax

		xor		ah, ah
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		inc		edi
		inc		ebx
		inc		edi
		inc		ebx

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferIntensityZ

        Creates a shadow using a brush, but modifies the destination buffer only
if the current Z level is equal to higher than what's in the Z buffer at that
pixel location. It updates the Z buffer with the new Z level.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferIntensityZ(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                        uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                                        int32_t iX, int32_t iY, uint16_t usIndex) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  (void)usHeight;
  UNIMPLEMENTED
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET IntensityTable
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL5

		xor		eax, eax
		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax
		mov		ax, usZValue
		mov		[ebx], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferIntensityZClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is
        updated to the current value,	for any non-transparent pixels. The
Z-buffer is 16 bit, and must be the same dimensions (including Pitch) as the
destination.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferIntensityZClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                            uint16_t *pZBuffer, uint16_t usZValue,
                                            HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                            uint16_t usIndex, SGPRect *clipregion) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  UNIMPLEMENTED
#else
  uint32_t Unblitted;
  int32_t LSCount;
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET IntensityTable
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL2

		mov		ax, usZValue
		mov		[ebx], ax

		xor		eax, eax

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferIntensityZNB

        Creates a shadow using a brush, but modifies the destination buffer only
if the current Z level is equal to higher than what's in the Z buffer at that
pixel location. It does NOT update the Z buffer with the new Z value.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferIntensityZNB(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                          uint16_t *pZBuffer, uint16_t usZValue,
                                          HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                          uint16_t usIndex) {
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  (void)usHeight;
  UNIMPLEMENTED
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET IntensityTable
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

BlitNTL4:

		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL5

		xor		eax, eax
		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferIntensityClip

        Modifies the destination buffer. Darkens the destination pixels by 25%,
using the source image as a mask. Any Non-zero index pixels are used to darken
destination pixels. Blitter clips brush if it doesn't fit on the viewport.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferIntensityClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                           HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                           uint16_t usIndex, SGPRect *clipregion) {
  uint8_t *DestPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  UNIMPLEMENTED
#else
  uint32_t Unblitted;
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET IntensityTable
		xor		eax, eax
		mov		ebx, TopSkip
		xor		ecx, ecx

		or		ebx, ebx  // check for nothing clipped on top
		jz		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		ebx
		jnz		TopSkipLoop




LeftSkipSetup:

		mov		Unblitted, 0
		mov		ebx, LeftSkip  // check for nothing clipped on the left
		or		ebx, ebx
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ebx
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, ebx  // skip partial run, jump into normal loop for rest
		sub		ecx, ebx
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		ebx, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, ebx
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, ebx  // skip partial run, jump into normal loop for rest
		mov		ebx, BlitLength
		jmp		BlitTransparent


LSTrans1:
		sub		ebx, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop




BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		ebx, BlitLength
		mov		Unblitted, 0

BlitDispatch:

		or		ebx, ebx  // Check to see if we're done blitting
		jz		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:

		cmp		ecx, ebx
		jbe		BNTrans1

		sub		ecx, ebx
		mov		Unblitted, ecx
		mov		ecx, ebx

BNTrans1:
		sub		ebx, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitLineEnd

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
		cmp		ecx, ebx
		jbe		BTrans1

		mov		ecx, ebx

BTrans1:

		sub		ebx, ecx
         //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferIntensity

        Modifies the destination buffer. Darkens the destination pixels by 25%,
using the source image as a mask. Any Non-zero index pixels are used to darken
destination pixels.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferIntensity(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                       HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                       uint16_t usIndex) {
  uint8_t *DestPtr;
  uint32_t LineSkip;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  // Validations
  CHECKV(iTempX >= 0);
  CHECKV(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr = (uint8_t *)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
  LineSkip = (uiDestPitchBYTES - (usWidth * 2));

#if 1  // XXX TODO
  (void)SrcPtr;
  (void)usHeight;
  UNIMPLEMENTED
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, usHeight
		xor		ecx, ecx
		mov		edx, OFFSET IntensityTable

BlitDispatch:


		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		xor		eax, eax

		add		esi, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		edi, 8
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		ebx
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZClipPixelateObscured

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is NOT updated to the current value,
for any non-transparent pixels. The Z-buffer is 16 bit, and must be the same
dimensions (including Pitch) as the destination.

        Blits every second pixel ("pixelates").

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransZClipPixelateObscured(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion) {
  uint32_t uiLineFlag;
  uint32_t Unblitted;
  uint8_t *DestPtr, *ZPtr;
  uint32_t LineSkip;
  int32_t LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
  int32_t ClipX1, ClipY1, ClipX2, ClipY2;

  // Assertions
  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t const usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  RightSkip = std::min(std::max(ClipX2, (iTempX + (int32_t)usWidth)) - ClipX2, (int32_t)usWidth);
  TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  BottomSkip = std::min(std::max(ClipY2, (iTempY + (int32_t)usHeight)) - ClipY2, (int32_t)usHeight);

  // calculate the remaining rows and columns to blit
  BlitLength = ((int32_t)usWidth - LeftSkip - RightSkip);
  BlitHeight = ((int32_t)usHeight - TopSkip - BottomSkip);

  // check if whole thing is clipped
  if ((LeftSkip >= (int32_t)usWidth) || (RightSkip >= (int32_t)usWidth)) return;

  // check if whole thing is clipped
  if ((TopSkip >= (int32_t)usHeight) || (BottomSkip >= (int32_t)usHeight)) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  DestPtr =
      (uint8_t *)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  ZPtr = (uint8_t *)pZBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();
  LineSkip = (uiDestPitchBYTES - (BlitLength * 2));
  uiLineFlag = (iTempY & 1);

#if 1  // XXX TODO
  uiLineFlag = (iTempY + TopSkip) & 1;

  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr < usZValue) {
            *(uint16_t *)ZPtr = usZValue;
          } else {
            if (uiLineFlag != (((uintptr_t)DestPtr & 2) != 0)) continue;
          }
          *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
        } while (SrcPtr++, DestPtr += 2, ZPtr += 2, --PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
    uiLineFlag ^= 1;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop:  // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		xor		uiLineFlag, 1
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop


BlitLineSetup:  // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:  // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:

        // OK, DO CHECK FOR Z FIRST!
		mov		ax, [ebx]
		cmp		ax, usZValue
		jae		BlitNTL8

        // ONLY WRITE DATA IF WE REALLY SHOULD
		mov		ax, usZValue
		mov		[ebx], ax
		jmp   BlitNTL7

BlitNTL8:

		test	uiLineFlag, 1
		jz		BlitNTL6

		test	edi, 2
		jz		BlitNTL2
		jmp		BlitNTL7

BlitNTL6:
		test	edi, 2
		jnz		BlitNTL2

BlitNTL7:

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2
		dec		cl
		jnz		BlitNTL1

        // BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:

		sub		LSCount, ecx
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


RightSkipLoop:    // skip along until we hit and end-of-line marker


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		xor		uiLineFlag, 1
		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}
