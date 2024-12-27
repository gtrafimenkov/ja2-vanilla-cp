// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef SMACK_STUB_H
#define SMACK_STUB_H

#include "SGP/Types.h"
extern "C" {
#include "libsmacker/smacker.h"
}

enum { DISABLE = 0, ENABLE };

enum { SMACKBUFFER555 = 0x80000000, SMACKBUFFER565 = 0xC0000000 };

enum { SMACKAUTOBLIT = 0 };

enum {
  SMACKFILEHANDLE = 0x01000,
  SMACKTRACK1 = 0x02000,
  SMACKTRACK2 = 0x04000,
  SMACKTRACK3 = 0x08000,
  SMACKTRACK4 = 0x10000,
  SMACKTRACK5 = 0x20000,
  SMACKTRACK6 = 0x40000,
  SMACKTRACK7 = 0x80000,
  SMACKTRACKS = SMACKTRACK1 | SMACKTRACK2 | SMACKTRACK3 | SMACKTRACK4 | SMACKTRACK5 | SMACKTRACK6 |
                SMACKTRACK7,

  SMACKAUTOEXTRA = 0xFFFFFFFF
};

struct Smack {
  smk Smacker;  // object pointer type for libsmacker
  unsigned char *SmackerInMemory;
  uint32_t SoundTag;  // for soundman
  uint32_t Height;
  uint32_t Width;
  uint32_t Frames;
  uint32_t FramesPerSecond;
  uint32_t LastTick;
};

typedef void SmackBuf;

Smack *SmackOpen(SGPFile *fhandle, uint32_t Flags, uint32_t ExtraFlag);
uint32_t SmackDoFrame(Smack *Smk);
char SmackNextFrame(Smack *Smk);

uint32_t SmackWait(Smack *Smk);
uint32_t SmackSkipFrames(Smack *Smk);
void SmackClose(Smack *Smk);

void SmackToBuffer(Smack *Smk, uint32_t Left, uint32_t Top, uint32_t Pitch, uint32_t DestHeight,
                   uint32_t DestWidth, void *Buf, uint32_t Flags);

// SDL_Surface* SmackBufferOpen(uint32_t BlitType, uint32_t Width, uint32_t Height,
// uint32_t ZoomW, uint32_t ZoomH); void SmackBufferClose(SmackBuf* SBuf);

uint32_t SmackUseMMX(uint32_t Flag);

#endif
