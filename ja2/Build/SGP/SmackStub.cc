// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.


#include "SGP/SmackStub.h"

#include <stdio.h>
#include <string.h>

#include "SGP/FileMan.h"
#include "SGP/SoundMan.h"
#include "Utils/SoundControl.h"

extern "C" {
#include "libsmacker/smacker.h"
}

#include "SDL_timer.h"

#define SMKTRACK 0

typedef unsigned char UCHAR8;

BOOLEAN SmackCheckStatus(char smkstatus) {
  if (smkstatus < 0) {
    printf("SmackLibrary returned an error!\n");
    return TRUE;
  }
  return FALSE;
}

BOOLEAN SmkVideoSwitch(smk SmkObj, BOOLEAN sw) {
  return (SmackCheckStatus(smk_enable_video(SmkObj, sw)));
}

BOOLEAN SmkAudioSwitch(smk SmkObj, BOOLEAN sw) {
  return SmackCheckStatus(smk_enable_audio(SmkObj, SMKTRACK, sw));
}

void SmackPrintFlickInfo(unsigned long width, unsigned long height, UCHAR8 scale,
                         unsigned long framecount, double usf, UCHAR8 a_channels, UCHAR8 a_depth,
                         UCHAR8 a_rate) {
  printf("Video -- Frames: %lu Width: %lu Height: %lu Scale: %d \n", framecount, width, height,
         scale);
  printf("Audio -- FPS: %2.2f  Channels: %d Depth: %d Rate %d\n", usf / 1000, a_channels, a_depth,
         a_rate);
}

// read all smackaudio and convert it to 22050Hz on the fly (44100 originally)
uint32_t SmackGetAudio(const smk SmkObj, const int16_t *audiobuffer) {
  uint32_t n_samples = 0, smacklen = 0;
  uint32_t i, index;
  int16_t *smackaudio, *paudio = (int16_t *)audiobuffer;
  if (!audiobuffer) return 0;
  SmkAudioSwitch(SmkObj, ENABLE);
  smk_first(SmkObj);
  do {
    smacklen = smk_get_audio_size(SmkObj, SMKTRACK);
    smackaudio = (int16_t *)smk_get_audio(SmkObj, SMKTRACK);
    index = 0;
    for (i = 0; i < smacklen / 8; i++) {
      *paudio++ = ((smackaudio[index] + smackaudio[index + 2]) / 2);
      *paudio++ = ((smackaudio[index + 1] + smackaudio[index + 3]) / 2);
      index += 4;
    }
    n_samples += i;
  } while (smk_next(SmkObj) != SMK_DONE);
  SmkAudioSwitch(SmkObj, DISABLE);
  return n_samples;
}

void SmackWriteAudio(int16_t *abuffer, uint32_t size) {
  FILE *fp = fopen("/tmp/smk.raw", "wb");
  fwrite(abuffer, size, 1, fp);
  fclose(fp);
}

UCHAR8 *SmackToMemory(SGPFile *File) {
  UCHAR8 *smacktomemory;
  uint32_t FileSize = FileGetSize(File);

  smacktomemory = (UCHAR8 *)malloc(FileSize);
  if (!smacktomemory) return NULL;
  FileRead(File, smacktomemory, FileSize);
  return smacktomemory;
}

Smack *SmackOpen(SGPFile *FileHandle, uint32_t Flags, uint32_t ExtraFlag) {
  Smack *flickinfo;
  // smacklib info types
  unsigned long frame, framecount, width, height;
  UCHAR8 scale;
  double usf;
  char smkstatus;
  /* arrays for audio track metadata */
  UCHAR8 a_depth[7], a_channels[7];
  unsigned long a_rate[7];
  unsigned long audiolen, audiosamples;
  int16_t *audiobuffer;
  UCHAR8 *smackloaded;
  uint32_t smacksize = FileGetSize(FileHandle);
  flickinfo = (Smack *)malloc(sizeof(Smack));
  if (!flickinfo) return NULL;

  smackloaded = SmackToMemory(FileHandle);
  if (!smackloaded) {
    free(flickinfo);
    return NULL;
  }
  flickinfo->SmackerInMemory = smackloaded;
  flickinfo->Smacker = smk_open_memory(smackloaded, smacksize);
  // open file with given filehandle DISK/MEMORY mode
  // flickinfo->Smacker = smk_open_generic(1, fp, 0, SMK_MODE_DISK);
  // flickinfo->Smacker = smk_open_generic(1, fp, 0, SMK_MODE_MEMORY);
  if (!flickinfo->Smacker) {
    free(flickinfo);
    return NULL;
  }

  smkstatus = smk_info_video(flickinfo->Smacker, &width, &height, &scale);
  SmackCheckStatus(smkstatus);
  smkstatus = smk_info_all(flickinfo->Smacker, &frame, &framecount, &usf);
  SmackCheckStatus(smkstatus);
  smkstatus = smk_info_audio(flickinfo->Smacker, NULL, a_channels, a_depth, a_rate);
  SmackCheckStatus(smkstatus);

  SmkVideoSwitch(flickinfo->Smacker, DISABLE);

  flickinfo->Frames = framecount;
  flickinfo->Height = height;
  flickinfo->Width = width;
  flickinfo->FramesPerSecond = usf;
  // calculated audio memory for downsampling 44100->22050
  audiosamples =
      ((flickinfo->Frames / (usf / 1000)) * (a_rate[SMKTRACK] / 2) * 16 * a_channels[SMKTRACK]);
  audiobuffer = (int16_t *)malloc(audiosamples);
  if (!audiobuffer) {
    free(flickinfo);
    return NULL;
  }
  audiolen = SmackGetAudio(flickinfo->Smacker, audiobuffer);
  // SmackWriteAudio( audiobuffer, audiolen); // are getting right audio data?
  // shoot and forget... audiobuffer should be freed by SoundMan
  if (audiolen > 0)
    flickinfo->SoundTag = SoundPlayFromBuffer(audiobuffer, audiolen, MAXVOLUME, 64, 1, NULL, NULL);
  else
    free(audiobuffer), audiobuffer = NULL;
  SmkVideoSwitch(flickinfo->Smacker, ENABLE);
  if ((smk_first(flickinfo->Smacker) < 0)) {
    printf("First Failed!");
    return NULL;
  }
  smk_first(flickinfo->Smacker);
  flickinfo->LastTick = SDL_GetTicks();
  return flickinfo;
}

uint32_t SmackDoFrame(Smack *Smk) {
  uint32_t i = 0;
  // wait for FPS milliseconds
  uint16_t millisecondspassed = SDL_GetTicks() - Smk->LastTick;
  uint16_t skiptime;
  uint16_t delay, skipframes = 0;
  double framerate = Smk->FramesPerSecond / 1000;

  if (framerate > millisecondspassed) {
    delay = framerate - millisecondspassed;
  } else  // video is delayed - so skip frames according to delay but take fps
          // into account
  {
    skipframes = millisecondspassed / (uint16_t)framerate;
    delay = millisecondspassed % (uint16_t)framerate;
    // bigskiptime:
    skiptime = SDL_GetTicks();
    millisecondspassed = skiptime - Smk->LastTick;
    while (skipframes > 0) {
      SmackNextFrame(Smk);
      skipframes--;
      i++;
    }
    skiptime = SDL_GetTicks() - skiptime;
    if (skiptime + delay <= i * framerate) {
      delay = i * (framerate)-skiptime - delay;
    } else {
      delay = 0;
      // need to find a nice way to compensate for lagging video
      // Smk->LastTick = SDL_GetTicks();
      // skipframes = skiptime+delay / (uint16_t)framerate;
      // delay = (skiptime+delay) % (uint16_t)framerate;
      // i=0;
      // goto bigskiptime; // skiptime was big.. so just go on skipping frames
    }
  }
  SDL_Delay(delay);
  Smk->LastTick = SDL_GetTicks();
  return 0;
}

char SmackNextFrame(Smack *Smk) {
  char smkstatus;
  smkstatus = smk_next(Smk->Smacker);
  return smkstatus;
}

uint32_t SmackWait(Smack *Smk) { return 0; }

void SmackClose(Smack *Smk) {
  if (!SoundStop(Smk->SoundTag)) printf("Error in SmackClose SoundStop\n");
  free(Smk->SmackerInMemory);
  smk_close(Smk->Smacker);  // closes and frees Smacker Object and file
}

void SmackToBuffer(Smack *Smk, uint32_t Left, uint32_t Top, uint32_t Pitch, uint32_t DestHeight,
                   uint32_t DestWidth, void *Buf, uint32_t Flags) {
  unsigned char *smackframe, *pframe;
  unsigned char *smackpal;
  uint16_t i, j, pixel, *buf;
  uint8_t *rgb;
  uint32_t halfpitch = Pitch / 2;
  smackframe = smk_get_video(Smk->Smacker);
  smackpal = smk_get_palette(Smk->Smacker);
  // dump_bmp (smackpal, smackframe, 640, 480, Smk->FrameNum);
  buf = (uint16_t *)Buf;
  pframe = smackframe;
  // hardcoded copy to frambuffer without taking sdl methods into account.
  // maybe need to find a way to blit it later
  if (Flags == SMACKBUFFER565) {
    buf += Left + Top * halfpitch;
    for (i = 0; i < DestHeight; i++) {
      for (j = 0; j < DestWidth; j++) {
        rgb = &smackpal[*pframe++ * 3];
        *buf++ = (rgb[0] >> 3) << 11 | (rgb[1] >> 2) << 5 | rgb[2] >> 3;
      }
      buf += halfpitch - DestWidth;
    }
  } else  // SMACKBUFFER555
  {
    for (i = 0; i < DestHeight; i++) {
      for (j = 0; j < DestWidth; j++) {
        // get rgb offset of palette
        rgb = &smackpal[pframe[0] * 3];
        // convert from rbg to rgb555 0=red 1=green 2=blue
        pixel = (rgb[0] >> 3) << 10 | (rgb[1] >> 2) << 5 | rgb[2] >> 3;
        buf[(j + Top) + (i + Left) * halfpitch] = pixel;
        pframe++;
      }
    }
  }
}

// not needed for now...
/*
SDL_Surface* SmackBufferOpen(uint32_t BlitType, uint32_t Width, uint32_t Height,
uint32_t ZoomW, uint32_t ZoomH)
{
  SDL_Surface* buffer;
  buffer = SDL_CreateRGBSurface(BlitType, Width, Height, 16, 0, 0, 0, 0);
  if ( ! buffer ) {
    fprintf(stderr, "CreateRGBSurface failed: %s       ", SDL_GetError());
    exit (1);
  }
  return buffer;
}
*/

/*
void SmackBufferClose(SmackBuf* SBuf)
{
  free (SBuf);
}
*/

uint32_t SmackUseMMX(uint32_t Flag) { return 0; }
