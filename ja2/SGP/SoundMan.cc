// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/SoundMan.h"

#include <SDL.h>
#include <algorithm>
#include <assert.h>
#include <string.h>

#include "Macro.h"
#include "SGP/Buffer.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Random.h"
#include "SGP/Timer.h"

// Uncomment this to disable the startup of sound hardware
// #define SOUND_DISABLE

#ifdef WITH_SOUND_DEBUG
#define SNDDBG(fmt, ...) () fprintf(stderr, ">>>> SND: " fmt, __VA_ARGS__)
#else
#define SNDDBG(fmt, ...) (void)0
#endif

/*
 * from\to FREE PLAY STOP DEAD
 *    FREE       M
 *    PLAY  2         M    C
 *    STOP  2              C
 *    DEAD  M         1
 *
 * M = Regular state transition done by main thread
 * C = Regular state transition done by sound callback
 * 1 = Unimportant race, dead channel can be marked stopped by main thread
 *     Gets marked as dead again in the next sound callback run
 * 2 = Only when stopping all sounds, sound callback is deactivated when this
 *     happens
 */
enum { CHANNEL_FREE, CHANNEL_PLAY, CHANNEL_STOP, CHANNEL_DEAD };

// Sample status flags
enum {
  SAMPLE_ALLOCATED = 1U << 0,
  SAMPLE_LOCKED = 1U << 1,
  SAMPLE_RANDOM = 1U << 2,
  SAMPLE_STEREO = 1U << 3,
  SAMPLE_16BIT = 1U << 4
};

#define SOUND_MAX_CACHED 128  // number of cache slots

#define SOUND_MAX_CHANNELS 16  // number of mixer channels

#define SOUND_DEFAULT_MEMORY (16 * 1024 * 1024)  // default memory limit
#define SOUND_DEFAULT_THRESH (2 * 1024 * 1024)   // size for sample to be double-buffered
#define SOUND_DEFAULT_STREAM (64 * 1024)         // double-buffered buffer size

// Struct definition for sample slots in the cache
// Holds the regular sample data, as well as the data for the random samples
struct SAMPLETAG {
  char pName[128];  // Path to sample data
  uint32_t n_samples;
  uint32_t uiFlags;  // Status flags
  uint32_t uiSpeed;  // Playback frequency
  void *pData;       // pointer to sample data memory
  uint32_t uiCacheHits;

  // Random sound data
  uint32_t uiTimeNext;
  uint32_t uiTimeMin;
  uint32_t uiTimeMax;
  uint32_t uiVolMin;
  uint32_t uiVolMax;
  uint32_t uiPanMin;
  uint32_t uiPanMax;
  uint32_t uiInstances;
  uint32_t uiMaxInstances;
};

// Structure definition for slots in the sound output
// These are used for both the cached and double-buffered streams
struct SOUNDTAG {
  volatile uint32_t State;
  SAMPLETAG *pSample;
  uint32_t uiSoundID;
  void (*EOSCallback)(void *);
  void *pCallbackData;
  uint32_t uiTimeStamp;
  HWFILE hFile;
  uint32_t uiFadeVolume;
  uint32_t uiFadeRate;
  uint32_t uiFadeTime;
  uint32_t pos;
  uint32_t Loops;
  uint32_t Pan;
};

static const uint32_t guiSoundDefaultVolume = MAXVOLUME;
static const uint32_t guiSoundMemoryLimit = SOUND_DEFAULT_MEMORY;  // Maximum memory used for sounds
static uint32_t guiSoundMemoryUsed = 0;                            // Memory currently in use
static const uint32_t guiSoundCacheThreshold = SOUND_DEFAULT_THRESH;  // Double-buffered threshold

static BOOLEAN fSoundSystemInit = FALSE;  // Startup called
static BOOLEAN gfEnableStartup = TRUE;    // Allow hardware to start up

// Sample cache list for files loaded
static SAMPLETAG pSampleList[SOUND_MAX_CACHED];
// Sound channel list for output channels
static SOUNDTAG pSoundList[SOUND_MAX_CHANNELS];

void SoundEnableSound(BOOLEAN fEnable) { gfEnableStartup = fEnable; }

static void SoundInitCache();
static BOOLEAN SoundInitHardware();

void InitializeSoundManager() {
  if (fSoundSystemInit) ShutdownSoundManager();

  memset(pSoundList, 0, sizeof(pSoundList));

#ifndef SOUND_DISABLE
  if (gfEnableStartup && SoundInitHardware()) fSoundSystemInit = TRUE;
#endif

  SoundInitCache();

  guiSoundMemoryUsed = 0;
}

static void SoundEmptyCache();
static void SoundShutdownHardware();

void ShutdownSoundManager() {
  SoundStopAll();
  SoundEmptyCache();
  SoundShutdownHardware();
  fSoundSystemInit = FALSE;
}

static SOUNDTAG *SoundGetFreeChannel();
static SAMPLETAG *SoundLoadSample(const char *pFilename);
static BOOLEAN SoundPlayStreamed(const char *pFilename);
static uint32_t SoundStartSample(SAMPLETAG *sample, SOUNDTAG *channel, uint32_t volume,
                                 uint32_t pan, uint32_t loop, void (*end_callback)(void *),
                                 void *data);

uint32_t SoundPlay(const char *pFilename, uint32_t volume, uint32_t pan, uint32_t loop,
                   void (*end_callback)(void *), void *data) {
  if (!fSoundSystemInit) return SOUND_ERROR;

#if 0  // TODO0003 implement streaming
	if (SoundPlayStreamed(pFilename))
	{
		//Trying to play a sound which is bigger then the 'guiSoundCacheThreshold'

		// This line was causing a page fault in the Wiz 8 project, so
		// I changed it to the second line, which works OK. -- DB

		//DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("\n*******\nSoundPlay():  ERROR:  trying to play %s which is bigger then the 'guiSoundCacheThreshold', use SoundPlayStreamedFile() instead\n", pFilename));

		FastDebugMsg(String("SoundPlay: ERROR: Trying to play %s sound is too lardge to load into cache, use SoundPlayStreamedFile() instead\n", pFilename));
		return SOUND_ERROR;
	}
#endif

  SAMPLETAG *const sample = SoundLoadSample(pFilename);
  if (sample == NULL) return SOUND_ERROR;

  SOUNDTAG *const channel = SoundGetFreeChannel();
  if (channel == NULL) return SOUND_ERROR;

  return SoundStartSample(sample, channel, volume, pan, loop, end_callback, data);
}

static SAMPLETAG *SoundGetEmptySample();
static BOOLEAN SoundCleanCache();
static SAMPLETAG *SoundGetEmptySample();
static size_t GetSampleSize(const SAMPLETAG *const s);

uint32_t SoundPlayFromBuffer(int16_t *pbuffer, uint32_t size, uint32_t volume, uint32_t pan,
                             uint32_t loop, void (*end_callback)(void *), void *data) {
  // SoundCleanCache();
  SAMPLETAG *buffertag = SoundGetEmptySample();
  if (buffertag == NULL) {
    SoundCleanCache();
    buffertag = SoundGetEmptySample();
  }
  sprintf(buffertag->pName, "SmackBuff %p - SampleSize %u", pbuffer, size);
  buffertag->uiSpeed = 22050;
  buffertag->n_samples = size;
  buffertag->pData = pbuffer;
  buffertag->uiFlags = SAMPLE_16BIT | SAMPLE_STEREO | SAMPLE_ALLOCATED;
  buffertag->uiPanMax = 64;
  buffertag->uiMaxInstances = 1;
  guiSoundMemoryUsed += size * GetSampleSize(buffertag);

  SOUNDTAG *const channel = SoundGetFreeChannel();
  if (channel == NULL) return SOUND_ERROR;

  return SoundStartSample(buffertag, channel, volume, pan, loop, end_callback, data);
}

static uint32_t SoundStartStream(const char *pFilename, SOUNDTAG *channel, uint32_t volume,
                                 uint32_t pan, uint32_t loop, void (*end_callback)(void *),
                                 void *data);

uint32_t SoundPlayStreamedFile(const char *pFilename, uint32_t volume, uint32_t pan, uint32_t loop,
                               void (*end_callback)(void *), void *data) try {
#if 1
  // TODO0003 implement streaming
  return SoundPlay(pFilename, volume, pan, loop, end_callback, data);
#else
  if (!fSoundSystemInit) return SOUND_ERROR;

  SOUNDTAG *const channel = SoundGetFreeChannel();
  if (channel == NULL) return SOUND_ERROR;

  AutoSGPFile hFile(FileMan::openForReadingSmart(pFilename, true));

  // MSS cannot determine which provider to play if you don't give it a real
  // filename so if the file isn't in a library, play it normally
  if (DB_EXTRACT_LIBRARY(hFile) == REAL_FILE_LIBRARY_ID) {
    return SoundStartStream(pFilename, channel, volume, pan, loop, end_callback, data);
  }

  // Get the real file handle of the file
  FILE *hRealFileHandle = GetRealFileHandleFromFileManFileHandle(hFile);
  if (hRealFileHandle == NULL) {
    FastDebugMsg(
        String("\n*******\nSoundPlayStreamedFile():  ERROR:  Couldnt get a "
               "real file handle for '%s' in SoundPlayStreamedFile()\n",
               pFilename));
    return SOUND_ERROR;
  }

  // Convert the file handle into a 'name'
  char pFileHandlefileName[128];
  sprintf(pFileHandlefileName, "\\\\\\\\%d", hRealFileHandle);

  // Start the sound stream
  uint32_t uiRetVal =
      SoundStartStream(pFileHandlefileName, channel, volume, pan, loop, end_callback, data);

  // if it succeeded, record the file handle
  if (uiRetVal != SOUND_ERROR) {
    channel->hFile = hFile.Release();
  }

  return uiRetVal;
#endif
} catch (...) {
  FastDebugMsg(
      String("\n*******\nSoundPlayStreamedFile():  ERROR:  Failed to play '%s'\n", pFilename));
  return SOUND_ERROR;
}

uint32_t SoundPlayRandom(const char *pFilename, uint32_t time_min, uint32_t time_max,
                         uint32_t vol_min, uint32_t vol_max, uint32_t pan_min, uint32_t pan_max,
                         uint32_t max_instances) {
  SNDDBG("RAND \"%s\"\n", pFilename);

  if (!fSoundSystemInit) return SOUND_ERROR;

  SAMPLETAG *const s = SoundLoadSample(pFilename);
  if (s == NULL) return SOUND_ERROR;

  s->uiFlags |= SAMPLE_RANDOM | SAMPLE_LOCKED;
  s->uiTimeMin = time_min;
  s->uiTimeMax = time_max;
  s->uiVolMin = vol_min;
  s->uiVolMax = vol_max;
  s->uiPanMin = pan_min;
  s->uiPanMax = pan_max;
  s->uiMaxInstances = max_instances;

  s->uiTimeNext = GetClock() + s->uiTimeMin + Random(s->uiTimeMax - s->uiTimeMin);

  return (uint32_t)(s - pSampleList);
}

static SOUNDTAG *SoundGetChannelByID(uint32_t uiSoundID);

BOOLEAN SoundIsPlaying(uint32_t uiSoundID) {
  if (!fSoundSystemInit) return FALSE;

  const SOUNDTAG *const channel = SoundGetChannelByID(uiSoundID);
  return channel != NULL && channel->State != CHANNEL_FREE;
}

static BOOLEAN SoundStopChannel(SOUNDTAG *channel);

BOOLEAN SoundStop(uint32_t uiSoundID) {
  if (!fSoundSystemInit) return FALSE;
  if (!SoundIsPlaying(uiSoundID)) return FALSE;

  SOUNDTAG *const channel = SoundGetChannelByID(uiSoundID);
  if (channel == NULL) return FALSE;

  SoundStopChannel(channel);
  return TRUE;
}

void SoundStopAll() {
  if (!fSoundSystemInit) return;

  SDL_PauseAudio(1);
  FOR_EACH(SOUNDTAG, i, pSoundList) {
    if (SoundStopChannel(i)) {
      assert(i->pSample->uiInstances != 0);
      i->pSample->uiInstances -= 1;
      i->pSample = NULL;
      i->uiSoundID = SOUND_ERROR;
      i->State = CHANNEL_FREE;
    }
  }
  SDL_PauseAudio(0);
}

BOOLEAN SoundSetVolume(uint32_t uiSoundID, uint32_t uiVolume) {
  if (!fSoundSystemInit) return FALSE;

  SOUNDTAG *const channel = SoundGetChannelByID(uiSoundID);
  if (channel == NULL) return FALSE;

  channel->uiFadeVolume = std::min(uiVolume, (uint32_t)MAXVOLUME);
  return TRUE;
}

BOOLEAN SoundSetPan(uint32_t uiSoundID, uint32_t uiPan) {
  if (!fSoundSystemInit) return FALSE;

  SOUNDTAG *const channel = SoundGetChannelByID(uiSoundID);
  if (channel == NULL) return FALSE;

  channel->Pan = std::min(uiPan, (uint32_t)127);
  return TRUE;
}

uint32_t SoundGetVolume(uint32_t uiSoundID) {
  if (!fSoundSystemInit) return SOUND_ERROR;

  const SOUNDTAG *const channel = SoundGetChannelByID(uiSoundID);
  if (channel == NULL) return SOUND_ERROR;

  return channel->uiFadeVolume;
}

static BOOLEAN SoundRandomShouldPlay(const SAMPLETAG *s);
static uint32_t SoundStartRandom(SAMPLETAG *s);

void SoundServiceRandom() {
  FOR_EACH(SAMPLETAG, i, pSampleList) {
    if (SoundRandomShouldPlay(i)) SoundStartRandom(i);
  }
}

/* Determines whether a random sound is ready for playing or not.
 *
 * Returns: TRUE if a the sample should be played. */
static BOOLEAN SoundRandomShouldPlay(const SAMPLETAG *s) {
  return s->uiFlags & SAMPLE_RANDOM && s->uiTimeNext <= GetClock() &&
         s->uiInstances < s->uiMaxInstances;
}

/* Starts an instance of a random sample.
 *
 * Returns: TRUE if a new random sound was created, FALSE if nothing was done.
 */
static uint32_t SoundStartRandom(SAMPLETAG *s) {
  SOUNDTAG *const channel = SoundGetFreeChannel();
  if (channel == NULL) return NO_SAMPLE;

  const uint32_t volume = s->uiVolMin + Random(s->uiVolMax - s->uiVolMin);
  const uint32_t pan = s->uiPanMin + Random(s->uiPanMax - s->uiPanMin);

  const uint32_t uiSoundID = SoundStartSample(s, channel, volume, pan, 1, NULL, NULL);
  if (uiSoundID == SOUND_ERROR) return NO_SAMPLE;

  s->uiTimeNext = GetClock() + s->uiTimeMin + Random(s->uiTimeMax - s->uiTimeMin);
  return uiSoundID;
}

void SoundStopAllRandom() {
  // Stop all currently playing random sounds
  FOR_EACH(SOUNDTAG, i, pSoundList) {
    if (i->State == CHANNEL_PLAY && i->pSample->uiFlags & SAMPLE_RANDOM) {
      SoundStopChannel(i);
    }
  }

  // Unlock all random sounds so they can be dumped from the cache, and
  // take the random flag off so they won't be serviced/played
  FOR_EACH(SAMPLETAG, i, pSampleList) {
    if (i->uiFlags & SAMPLE_RANDOM) {
      i->uiFlags &= ~(SAMPLE_RANDOM | SAMPLE_LOCKED);
    }
  }
}

void SoundServiceStreams() {
  if (!fSoundSystemInit) return;

  for (uint32_t i = 0; i < lengthof(pSoundList); i++) {
    SOUNDTAG *Sound = &pSoundList[i];
    if (Sound->State == CHANNEL_DEAD) {
      SNDDBG("DEAD channel %u file \"%s\" (refcount %u)\n", i, Sound->pSample->pName,
             Sound->pSample->uiInstances);
      if (Sound->EOSCallback != NULL) Sound->EOSCallback(Sound->pCallbackData);
      assert(Sound->pSample->uiInstances != 0);
      Sound->pSample->uiInstances--;
      Sound->pSample = NULL;
      Sound->uiSoundID = SOUND_ERROR;
      Sound->State = CHANNEL_FREE;
    }
  }
}

uint32_t SoundGetPosition(uint32_t uiSoundID) {
  if (!fSoundSystemInit) return 0;

  const SOUNDTAG *const channel = SoundGetChannelByID(uiSoundID);
  if (channel == NULL) return 0;

  const uint32_t now = GetClock();
  return now - channel->uiTimeStamp;
}

// Zeros out the structures of the sample list.
static void SoundInitCache() { memset(pSampleList, 0, sizeof(pSampleList)); }

static void SoundFreeSample(SAMPLETAG *s);

// Frees up all samples in the cache.
static void SoundEmptyCache() {
  SoundStopAll();

  FOR_EACH(SAMPLETAG, i, pSampleList) { SoundFreeSample(i); }
}

static SAMPLETAG *SoundGetCached(const char *pFilename);
static SAMPLETAG *SoundLoadDisk(const char *pFilename);

static SAMPLETAG *SoundLoadSample(const char *pFilename) {
  SAMPLETAG *const s = SoundGetCached(pFilename);
  if (s != NULL) return s;

  return SoundLoadDisk(pFilename);
}

/* Tries to locate a sound by looking at what is currently loaded in the cache.
 *
 * Returns: The sample index if successful, NO_SAMPLE if the file wasn't found
 *          in the cache. */
static SAMPLETAG *SoundGetCached(const char *pFilename) {
  if (pFilename[0] == '\0') return NULL;  // XXX HACK0009

  FOR_EACH(SAMPLETAG, i, pSampleList) {
    if (strcasecmp(i->pName, pFilename) == 0) return i;
  }

  return NULL;
}

static size_t GetSampleSize(const SAMPLETAG *const s) {
  return (s->uiFlags & SAMPLE_16BIT ? 2 : 1) * (s->uiFlags & SAMPLE_STEREO ? 2 : 1);
}

static BOOLEAN HalfSampleRate(SAMPLETAG *const s) {
  SNDDBG("SMPL \"%s\" from %uHz to %uHz\n", s->pName, s->uiSpeed, s->uiSpeed / 2);

  uint32_t const n_samples = s->n_samples / 2;
  void *const ndata = malloc(n_samples * GetSampleSize(s));
  if (ndata == NULL) return FALSE;
  void *const odata = s->pData;
  if (s->uiFlags & SAMPLE_16BIT) {
    int16_t *const dst = (int16_t *)ndata;
    const int16_t *const src = (const int16_t *)odata;
    if (s->uiFlags & SAMPLE_STEREO) {
      for (size_t i = 0; i < n_samples; ++i) {
        dst[2 * i + 0] = (src[4 * i + 0] + src[4 * i + 2]) / 2;
        dst[2 * i + 1] = (src[4 * i + 1] + src[4 * i + 3]) / 2;
      }
    } else {
      for (size_t i = 0; i < n_samples; ++i) {
        dst[i] = (src[2 * i] + src[2 * i + 1]) / 2;
      }
    }
  } else {
    uint8_t *const dst = (uint8_t *)ndata;
    const uint8_t *const src = (const uint8_t *)odata;
    if (s->uiFlags & SAMPLE_STEREO) {
      for (size_t i = 0; i < n_samples; ++i) {
        dst[2 * i + 0] = (src[4 * i + 0] + src[4 * i + 2]) / 2;
        dst[2 * i + 1] = (src[4 * i + 1] + src[4 * i + 3]) / 2;
      }
    } else {
      for (size_t i = 0; i < n_samples; ++i) {
        dst[i] = (src[2 * i] + src[2 * i + 1]) / 2;
      }
    }
  }
  s->pData = ndata;
  free(odata);

  s->n_samples = n_samples;
  s->uiSpeed /= 2;
  return TRUE;
}

#define FOURCC(a, b, c, d) \
  ((uint8_t)(d) << 24 | (uint8_t)(c) << 16 | (uint8_t)(b) << 8 | (uint8_t)(a))

enum WaveFormatTag {
  WAVE_FORMAT_UNKNOWN = 0x0000,
  WAVE_FORMAT_PCM = 0x0001,
  WAVE_FORMAT_DVI_ADPCM = 0x0011
};

static void LoadPCM(SAMPLETAG *const s, HWFILE const file, uint32_t const size) {
  SGP::Buffer<uint8_t> data(size);
  FileRead(file, data, size);

  s->n_samples = size / GetSampleSize(s);
  s->pData = data.Release();
}

static inline int Clamp(int min, int x, int max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static void LoadDVIADPCM(SAMPLETAG *const s, HWFILE const file, uint16_t const block_align) {
  s->uiFlags |= SAMPLE_16BIT;

  size_t CountSamples = s->n_samples;
  int16_t *const Data = (int16_t *)malloc(CountSamples * GetSampleSize(s));
  int16_t *D = Data;

  for (;;) {
    int16_t CurSample_;
    FileRead(file, &CurSample_, sizeof(CurSample_));

    uint8_t StepIndex_;
    FileRead(file, &StepIndex_, sizeof(StepIndex_));

    FileSeek(file, 1, FILE_SEEK_FROM_CURRENT);  // reserved byte

    int32_t CurSample = CurSample_;
    int32_t StepIndex = StepIndex_;

    *D++ = CurSample;
    if (--CountSamples == 0) {
      s->pData = Data;
      return;
    }

    uint32_t DataCount = block_align / 4;
    while (--DataCount != 0) {
      uint32_t DataWord;
      FileRead(file, &DataWord, sizeof(DataWord));
      for (uint32_t i = 0; i < 8; i++) {
        static const int16_t StepTable[] = {
            7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,
            23,    25,    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,
            73,    80,    88,    97,    107,   118,   130,   143,   157,   173,   190,   209,
            230,   253,   279,   307,   337,   371,   408,   449,   494,   544,   598,   658,
            724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
            2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
            7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
            22385, 24623, 27086, 29794, 32767};

        static const int8_t IndexTable[] = {-1, -1, -1, -1, 2, 4, 6, 8};

#if 1
        int32_t Diff = ((DataWord & 7) * 2 + 1) * StepTable[StepIndex] >> 3;
#else
        int32_t Diff = 0;
        if (DataWord & 4) Diff += StepTable[StepIndex];
        if (DataWord & 2) Diff += StepTable[StepIndex] >> 1;
        if (DataWord & 1) Diff += StepTable[StepIndex] >> 2;
        Diff += StepTable[StepIndex] >> 3;
#endif
        if (DataWord & 8) Diff = -Diff;
        CurSample = Clamp(-32768, CurSample + Diff, 32767);
        StepIndex = Clamp(0, StepIndex + IndexTable[DataWord & 7], 88);
        DataWord >>= 4;

        *D++ = CurSample;
        if (--CountSamples == 0) {
          s->pData = Data;
          return;
        }
      }
    }
  }
}

/* Loads a sound file from disk into the cache, allocating memory and a slot
 * for storage.
 *
 * Returns: The sample index if successful, NO_SAMPLE if the file wasn't found
 *          in the cache. */
static SAMPLETAG *SoundLoadDisk(const char *pFilename) try {
  Assert(pFilename != NULL);

  AutoSGPFile hFile(FileMan::openForReadingSmart(pFilename, true));

  uint32_t uiSize = FileGetSize(hFile);

  // if insufficient memory, start unloading old samples until either
  // there's nothing left to unload, or we fit
  while (uiSize + guiSoundMemoryUsed > guiSoundMemoryLimit) {
    if (!SoundCleanCache()) {
      SNDDBG("Not enough memory. Size: %u, Used: %u, Max: %u\n", uiSize, guiSoundMemoryUsed,
             guiSoundMemoryLimit);
      FastDebugMsg(
          String("SoundLoadDisk:  ERROR:  trying to play %s, not enough memory\n", pFilename));
      return NULL;
    }
  }

  // if all the sample slots are full, unloading one
  SAMPLETAG *s = SoundGetEmptySample();
  if (s == NULL) {
    SoundCleanCache();
    s = SoundGetEmptySample();
  }

  // if we still don't have a sample slot
  if (s == NULL) {
    FastDebugMsg(
        String("SoundLoadDisk:  ERROR: Trying to play %s, sound channels are full\n", pFilename));
    return NULL;
  }

  memset(s, 0, sizeof(*s));

  FileSeek(hFile, 12, FILE_SEEK_FROM_CURRENT);

  uint16_t FormatTag = WAVE_FORMAT_UNKNOWN;
  uint16_t BlockAlign = 0;
  for (;;) {
    uint32_t Tag;
    uint32_t Size;

    FileRead(hFile, &Tag, sizeof(Tag));
    FileRead(hFile, &Size, sizeof(Size));

    switch (Tag) {
      case FOURCC('f', 'm', 't', ' '): {
        uint16_t Channels;
        uint32_t Rate;
        uint16_t BitsPerSample;

        FileRead(hFile, &FormatTag, sizeof(FormatTag));
        FileRead(hFile, &Channels, sizeof(Channels));
        FileRead(hFile, &Rate, sizeof(Rate));
        FileSeek(hFile, 4, FILE_SEEK_FROM_CURRENT);
        FileRead(hFile, &BlockAlign, sizeof(BlockAlign));
        FileRead(hFile, &BitsPerSample, sizeof(BitsPerSample));
        SNDDBG("LOAD file \"%s\" format %u channels %u rate %u bits %u to slot %u\n", pFilename,
               FormatTag, Channels, Rate, BitsPerSample, s - pSampleList);
        switch (FormatTag) {
          case WAVE_FORMAT_PCM:
            break;

          case WAVE_FORMAT_DVI_ADPCM:
            FileSeek(hFile, 4, FILE_SEEK_FROM_CURRENT);
            break;

          default:
            return NULL;
        }

        s->uiSpeed = Rate;
        if (Channels != 1) s->uiFlags |= SAMPLE_STEREO;
        if (BitsPerSample == 16) s->uiFlags |= SAMPLE_16BIT;
        break;
      }

      case FOURCC('f', 'a', 'c', 't'): {
        uint32_t Samples;
        FileRead(hFile, &Samples, sizeof(Samples));
        s->n_samples = Samples;
        break;
      }

      case FOURCC('d', 'a', 't', 'a'): {
        switch (FormatTag) {
          case WAVE_FORMAT_PCM:
            LoadPCM(s, hFile, Size);
            goto sound_loaded;

          case WAVE_FORMAT_DVI_ADPCM:
            LoadDVIADPCM(s, hFile, BlockAlign);
            goto sound_loaded;

          default:
            return NULL;
        }
      }

      default:
        FileSeek(hFile, Size, FILE_SEEK_FROM_CURRENT);
        break;
    }
  }

sound_loaded:
  strcpy(s->pName, pFilename);
  if (s->uiSpeed == 44100 && !HalfSampleRate(s)) {
    free(s->pData);
    return NULL;
  }
  guiSoundMemoryUsed += s->n_samples * GetSampleSize(s);
  s->uiFlags |= SAMPLE_ALLOCATED;
  s->uiInstances = 0;
  return s;
} catch (...) {
  return 0;
}

static BOOLEAN SoundSampleIsPlaying(const SAMPLETAG *s);

/* Removes the least-used sound from the cache to make room.
 *
 * Returns: TRUE if a sample was freed, FALSE if none */
static BOOLEAN SoundCleanCache() {
  SAMPLETAG *candidate = NULL;

  FOR_EACH(SAMPLETAG, i, pSampleList) {
    if (i->uiFlags & SAMPLE_ALLOCATED && !(i->uiFlags & SAMPLE_LOCKED) &&
        (candidate == NULL || candidate->uiCacheHits > i->uiCacheHits)) {
      if (!SoundSampleIsPlaying(i)) candidate = i;
    }
  }

  if (candidate != NULL) {
    SNDDBG("FREE sample %u \"%s\" with %u hits\n", candidate - pSampleList, candidate->pName,
           candidate->uiCacheHits);
    SoundFreeSample(candidate);
    return TRUE;
  }

  return FALSE;
}

// Returns TRUE/FALSE that a sample is currently in use for playing a sound.
static BOOLEAN SoundSampleIsPlaying(const SAMPLETAG *s) { return s->uiInstances > 0; }

/* Returns an available sample.
 *
 * Returns: A free sample or NULL if none are left. */
static SAMPLETAG *SoundGetEmptySample() {
  FOR_EACH(SAMPLETAG, i, pSampleList) {
    if (!(i->uiFlags & SAMPLE_ALLOCATED)) return i;
  }

  return NULL;
}

// Frees up a sample referred to by its index slot number.
static void SoundFreeSample(SAMPLETAG *s) {
  if (!(s->uiFlags & SAMPLE_ALLOCATED)) return;

  assert(s->uiInstances == 0);

  guiSoundMemoryUsed -= s->n_samples * GetSampleSize(s);
  free(s->pData);
  memset(s, 0, sizeof(*s));
}

/* Searches out a sound instance referred to by its ID number.
 *
 * Returns: If the instance was found, the pointer to the channel.  NULL
 *          otherwise. */
static SOUNDTAG *SoundGetChannelByID(uint32_t uiSoundID) {
  FOR_EACH(SOUNDTAG, i, pSoundList) {
    if (i->uiSoundID == uiSoundID) return i;
  }

  return NULL;
}

static void SoundCallback(void *userdata, Uint8 *stream, int len) {
  SDL_memset(stream, 0, len);

  uint16_t *Stream = (uint16_t *)stream;

  // XXX TODO proper mixing, mainly clipping
  for (uint32_t i = 0; i < lengthof(pSoundList); i++) {
    SOUNDTAG *Sound = &pSoundList[i];

    switch (Sound->State) {
      default:
      case CHANNEL_FREE:
      case CHANNEL_DEAD:
        continue;

      case CHANNEL_STOP:
        Sound->State = CHANNEL_DEAD;
        continue;

      case CHANNEL_PLAY: {
        const SAMPLETAG *const s = Sound->pSample;
        const int32_t vol_l = Sound->uiFadeVolume * (127 - Sound->Pan) / MAXVOLUME;
        const int32_t vol_r = Sound->uiFadeVolume * (0 + Sound->Pan) / MAXVOLUME;
        size_t samples = len / 4;
        size_t amount;

      mixing:
        amount = std::min(samples, (size_t)(s->n_samples - Sound->pos));
        if (s->uiFlags & SAMPLE_16BIT) {
          if (s->uiFlags & SAMPLE_STEREO) {
            const int16_t *const src = (const int16_t *)s->pData + Sound->pos * 2;
            for (uint32_t i = 0; i < amount; ++i) {
              Stream[2 * i + 0] += src[2 * i + 0] * vol_l >> 7;
              Stream[2 * i + 1] += src[2 * i + 1] * vol_r >> 7;
            }
          } else {
            const int16_t *const src = (const int16_t *)s->pData + Sound->pos;
            for (uint32_t i = 0; i < amount; i++) {
              const int32_t data = src[i];
              Stream[2 * i + 0] += data * vol_l >> 7;
              Stream[2 * i + 1] += data * vol_r >> 7;
            }
          }
        } else {
          if (s->uiFlags & SAMPLE_STEREO) {
            const uint8_t *const src = (const uint8_t *)s->pData + Sound->pos * 2;
            for (uint32_t i = 0; i < amount; ++i) {
              Stream[2 * i + 0] += (src[2 * i + 0] - 128) * vol_l << 1;
              Stream[2 * i + 1] += (src[2 * i + 1] - 128) * vol_r << 1;
            }
          } else {
            const uint8_t *const src = (const uint8_t *)s->pData + Sound->pos;
            for (uint32_t i = 0; i < amount; ++i) {
              const int32_t data = (src[i] - 128) << 1;
              Stream[2 * i + 0] += data * vol_l;
              Stream[2 * i + 1] += data * vol_r;
            }
          }
        }

        Sound->pos += amount;
        if (Sound->pos == s->n_samples) {
          if (Sound->Loops != 1) {
            if (Sound->Loops != 0) --Sound->Loops;
            Sound->pos = 0;
            samples -= amount;
            if (samples != 0) goto mixing;
          } else {
            Sound->State = CHANNEL_DEAD;
          }
        }
      }
    }
  }
}

static BOOLEAN SoundInitHardware() {
  SDL_InitSubSystem(SDL_INIT_AUDIO);

  SDL_AudioSpec spec;
  spec.freq = 22050;
  spec.format = AUDIO_S16SYS;
  spec.channels = 2;
  spec.samples = 1024;
  spec.callback = SoundCallback;
  spec.userdata = NULL;

  if (SDL_OpenAudio(&spec, NULL) != 0) return FALSE;

  memset(pSoundList, 0, sizeof(pSoundList));
  SDL_PauseAudio(0);
  return TRUE;
}

static void SoundShutdownHardware() { SDL_QuitSubSystem(SDL_INIT_AUDIO); }

/* Finds an unused sound channel in the channel list.
 *
 * Returns: Pointer to a sound channel if one was found, NULL if not. */
static SOUNDTAG *SoundGetFreeChannel() {
  FOR_EACH(SOUNDTAG, i, pSoundList) {
    if (i->State == CHANNEL_FREE) return i;
  }

  return NULL;
}

static uint32_t SoundGetUniqueID();

/* Starts up a sample on the specified channel. Override parameters are passed
 * in through the structure pointer pParms. Any entry with a value of 0xffffffff
 * will be filled in by the system.
 *
 * Returns: Unique sound ID if successful, SOUND_ERROR if not. */
static uint32_t SoundStartSample(SAMPLETAG *sample, SOUNDTAG *channel, uint32_t volume,
                                 uint32_t pan, uint32_t loop, void (*end_callback)(void *),
                                 void *data) {
  SNDDBG("PLAY channel %u sample %u file \"%s\"\n", channel - pSoundList, sample - pSampleList,
         sample->pName);

  if (!fSoundSystemInit) return SOUND_ERROR;

  channel->uiFadeVolume = volume;
  channel->Loops = loop;
  channel->Pan = pan;
  channel->EOSCallback = end_callback;
  channel->pCallbackData = data;

  uint32_t uiSoundID = SoundGetUniqueID();
  channel->uiSoundID = uiSoundID;
  channel->pSample = sample;
  channel->uiTimeStamp = GetClock();
  channel->pos = 0;
  channel->State = CHANNEL_PLAY;

  sample->uiInstances++;
  sample->uiCacheHits++;

  return uiSoundID;
}

/* Starts up a stream on the specified channel. Override parameters are passed
 * in through the structure pointer pParms. Any entry with a value of 0xffffffff
 * will be filled in by the system.
 *
 * Returns: Unique sound ID if successful, SOUND_ERROR if not. */
static uint32_t SoundStartStream(const char *pFilename, SOUNDTAG *channel, uint32_t volume,
                                 uint32_t pan, uint32_t loop, void (*end_callback)(void *),
                                 void *data) {
#if 1  // XXX TODO
  FIXME
  return SOUND_ERROR;
#else
  if (!fSoundSystemInit) return SOUND_ERROR;

  channel->hMSSStream = AIL_open_stream(hSoundDriver, pFilename,
                                        SOUND_DEFAULT_STREAM) if (channel->hMSSStream == NULL) {
    SoundCleanCache();
    channel->hMSSStream = AIL_open_stream(hSoundDriver, pFilename, SOUND_DEFAULT_STREAM);
  }

  if (channel->hMSSStream == NULL) {
    char AILString[200];
    sprintf(AILString, "Stream Error: %s", AIL_last_error());
    DebugMsg(TOPIC_GAME, DBG_LEVEL_0, AILString);
    return SOUND_ERROR;
  }

  AIL_set_stream_volume(channel->hMSSStream, volume);
  AIL_set_stream_loop_count(channel->hMSSStream, loop);
  AIL_set_stream_pan(channel->hMSSStream, pan);

  AIL_start_stream(channel->hMSSStream);

  uint32_t uiSoundID = SoundGetUniqueID();
  channel->uiSoundID = uiSoundID;

  channel->EOSCallback = end_callback;
  channel->pCallbackData = data;

  channel->uiTimeStamp = GetClock();
  channel->uiFadeVolume = SoundGetVolumeIndex(uiChannel);

  return uiSoundID;
#endif
}

/* Returns a unique ID number with every call. Basically it's just a 32-bit
 * static value that is incremented each time. */
static uint32_t SoundGetUniqueID() {
  static uint32_t uiNextID = 0;

  if (uiNextID == SOUND_ERROR) uiNextID++;

  return uiNextID++;
}

/* Returns TRUE/FALSE whether a sound file should be played as a streamed
 * sample, or loaded into the cache. The decision is based on the size of the
 * file compared to the guiSoundCacheThreshold.
 *
 * Returns: TRUE if it should be streamed, FALSE if loaded. */
static BOOLEAN SoundPlayStreamed(const char *pFilename) try {
  AutoSGPFile hDisk(FileMan::openForReadingSmart(pFilename, true));
  return FileGetSize(hDisk) >= guiSoundCacheThreshold;
} catch (...) {
  return FALSE;
}

/* Stops a sound referred to by its channel.  This function is the only one
 * that should be deallocating sample handles. The random sounds have to have
 * their counters maintained, and using this as the central function ensures
 * that they stay in sync.
 *
 * Returns: TRUE if the sample was stopped, FALSE if it could not be found. */
static BOOLEAN SoundStopChannel(SOUNDTAG *channel) {
  if (!fSoundSystemInit) return FALSE;

  if (channel->pSample == NULL) return FALSE;

  SNDDBG("STOP channel %u\n", channel - pSoundList);
  channel->State = CHANNEL_STOP;
  return TRUE;
}

void SoundStopRandom(uint32_t uiSample) {
  // CHECK FOR VALID SAMPLE
  SAMPLETAG *const s = &pSampleList[uiSample];
  if (s->uiFlags & SAMPLE_ALLOCATED) {
    s->uiFlags &= ~SAMPLE_RANDOM;
  }
}
