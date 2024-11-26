#include "Tactical/Gap.h"

#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/LoadSaveData.h"
#include "SGP/MemMan.h"
#include "SGP/SGPStrings.h"
#include "SGP/SoundMan.h"
#include "Utils/SoundControl.h"

static void AudioGapListInit(const char *zSoundFile, AudioGapList *pGapList) {
  /* This procedure will load in the appropriate .gap file, corresponding to
   * the .wav file in szSoundEffects indexed by uiSampleNum.  The procedure
   * will then allocate and load in the AUDIO_GAP information, while counting
   * the number of elements loaded */

  // DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("File is %s",
  // szSoundEffects[uiSampleNum]));

  // strip .wav and change to .gap
  char sFileName[256];
  ReplacePath(sFileName, lengthof(sFileName), 0, zSoundFile, ".gap");

  try {
    AutoSGPFile f(FileMan::openForReadingSmart(sFileName, true));

    // gap file exists
    // now read in the AUDIO_GAPs
    const uint32_t size = FileGetSize(f);

    const uint32_t count = size / 8;
    if (count > 0) {
      uint8_t *data = MALLOCN(uint8_t, size);
      FileRead(f, data, size);

      AUDIO_GAP *const gaps = MALLOCN(AUDIO_GAP, count);

      pGapList->gaps = gaps;
      pGapList->end = gaps + count;

      const uint8_t *d = data;
      for (uint32_t i = 0; i < count; ++i) {
        uint32_t start;
        uint32_t end;

        EXTR_U32(d, start);
        EXTR_U32(d, end);

        gaps[i].start = start;
        gaps[i].end = end;

        DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Gap Start %d and Ends %d", start, end));
      }

      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Gap List Started From File %s", sFileName));

      MemFree(data);
      return;
    }
  } catch (...) { /* Handled below */
  }

  pGapList->gaps = NULL;
  pGapList->end = NULL;
}

void AudioGapListDone(AudioGapList *pGapList) {
  /* Free the array and nullify the pointers in the AudioGapList */
  MemFree(pGapList->gaps);
  pGapList->gaps = NULL;
  pGapList->end = NULL;
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Audio Gap List Deleted");
}

BOOLEAN PollAudioGap(uint32_t uiSampleNum, AudioGapList *pGapList) {
  /* This procedure will access the AudioGapList pertaining to the .wav about
   * to be played and returns whether there is a gap currently.  This is done
   * by going to the current AUDIO_GAP element in the AudioGapList, comparing
   * to see if the current time is between the start and end. If so, return
   * TRUE..if not and the start of the next element is not greater than
   * current time, set current to next and repeat ...if next elements start
   * is larger than current time, or no more elements..  return FALSE */

  if (!pGapList) {
    // no gap list, return
    return FALSE;
  }

  const AUDIO_GAP *i = pGapList->gaps;
  if (i == NULL) return FALSE;

  const uint32_t time = SoundGetPosition(uiSampleNum);
  // DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Sound Sample Time is %d", time));

  // Check to see if we have fallen behind.  If so, catch up
  const AUDIO_GAP *const end = pGapList->end;
  while (time > i->end) {
    if (++i == end) return FALSE;
  }

  // check to see if time is within the next AUDIO_GAPs start time
  if (i->start < time && time < i->end) {
    // we are within the time frame
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Gap Started at %d", time));
    return TRUE;
  } else {
    return FALSE;
  }
}

uint32_t PlayJA2GapSample(const char *zSoundFile, uint32_t ubVolume, uint32_t ubLoops,
                          uint32_t uiPan, AudioGapList *pData) {
  // Setup Gap Detection, if it is not null
  if (pData != NULL) AudioGapListInit(zSoundFile, pData);

  const uint32_t vol = CalculateSpeechVolume(ubVolume);
  return SoundPlayStreamedFile(zSoundFile, vol, uiPan, ubLoops, NULL, NULL);
}
