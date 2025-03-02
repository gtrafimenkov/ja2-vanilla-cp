// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Utils/MusicControl.h"

#include <algorithm>

#include "Directories.h"
#include "GameScreen.h"
#include "JAScreens.h"
#include "SGP/Debug.h"
#include "SGP/Random.h"
#include "SGP/Types.h"
#include "ScreenIDs.h"
#include "Strategic/CreatureSpreading.h"
#include "Strategic/StrategicMap.h"
#include "Utils/TimerControl.h"

static uint32_t uiMusicHandle = NO_SAMPLE;
static uint32_t uiMusicVolume = 50;
static BOOLEAN fMusicPlaying = FALSE;
static BOOLEAN fMusicFadingOut = FALSE;
static BOOLEAN fMusicFadingIn = FALSE;

static BOOLEAN gfMusicEnded = FALSE;

uint8_t gubMusicMode = 0;
static uint8_t gubOldMusicMode = 0;

static int8_t gbVictorySongCount = 0;
static int8_t gbDeathSongCount = 0;

static int8_t bNothingModeSong;
static int8_t bEnemyModeSong;
static int8_t bBattleModeSong;

static int8_t gbFadeSpeed = 1;

const char *const szMusicList[] = {
    MUSICDIR "/marimbad 2.wav", MUSICDIR "/menumix1.wav",  MUSICDIR "/nothing a.wav",
    MUSICDIR "/nothing b.wav",  MUSICDIR "/nothing c.wav", MUSICDIR "/nothing d.wav",
    MUSICDIR "/tensor a.wav",   MUSICDIR "/tensor b.wav",  MUSICDIR "/tensor c.wav",
    MUSICDIR "/triumph.wav",    MUSICDIR "/death.wav",     MUSICDIR "/battle a.wav",
    MUSICDIR "/tensor b.wav",   MUSICDIR "/creepy.wav",    MUSICDIR "/creature battle.wav"};

BOOLEAN gfForceMusicToTense = FALSE;
static BOOLEAN gfDontRestartSong = FALSE;

static BOOLEAN MusicFadeIn();
static BOOLEAN MusicStop();
static void MusicStopCallback(void *pData);

void MusicPlay(uint32_t uiNum) {
  if (fMusicPlaying) MusicStop();

  uiMusicHandle = SoundPlayStreamedFile(szMusicList[uiNum], 0, 64, 1, MusicStopCallback, NULL);

  if (uiMusicHandle != SOUND_ERROR) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Music PLay %d %d", uiMusicHandle, gubMusicMode));

    gfMusicEnded = FALSE;
    fMusicPlaying = TRUE;
    MusicFadeIn();
    return;
  }

  DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Music PLay %d %d", uiMusicHandle, gubMusicMode));
}

static void StartMusicBasedOnMode();

void MusicSetVolume(uint32_t uiVolume) {
  int32_t uiOldMusicVolume = uiMusicVolume;

  uiMusicVolume = std::min(uiVolume, (uint32_t)MAXVOLUME);

  if (uiMusicHandle != NO_SAMPLE) {
    // get volume and if 0 stop music!
    if (uiMusicVolume == 0) {
      gfDontRestartSong = TRUE;
      MusicStop();
    } else {
      SoundSetVolume(uiMusicHandle, uiMusicVolume);
    }
  } else {
    // If here, check if we need to re-start music
    // Have we re-started?
    if (uiMusicVolume > 0 && uiOldMusicVolume == 0) {
      StartMusicBasedOnMode();
    }
  }
}

//********************************************************************************
// MusicGetVolume
//
//		Gets the volume on the currently playing music.
//
//	Returns:	TRUE if the volume was set, FALSE if an error occurred
//
//********************************************************************************
uint32_t MusicGetVolume() { return (uiMusicVolume); }

//		Stops the currently playing music.
//
//	Returns:	TRUE if the music was stopped, FALSE if an error
// occurred
static BOOLEAN MusicStop() {
  if (uiMusicHandle != NO_SAMPLE) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Music Stop %d %d", uiMusicHandle, gubMusicMode));

    SoundStop(uiMusicHandle);
    fMusicPlaying = FALSE;
    uiMusicHandle = NO_SAMPLE;
    return (TRUE);
  }

  DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Music Stop %d %d", uiMusicHandle, gubMusicMode));

  return (FALSE);
}

//		Fades out the current song.
//
//	Returns:	TRUE if the music has begun fading, FALSE if an error
// occurred
static BOOLEAN MusicFadeOut() {
  if (uiMusicHandle != NO_SAMPLE) {
    fMusicFadingOut = TRUE;
    return (TRUE);
  }
  return (FALSE);
}

//		Fades in the current song.
//
//	Returns:	TRUE if the music has begun fading in, FALSE if an error
// occurred
static BOOLEAN MusicFadeIn() {
  if (uiMusicHandle != NO_SAMPLE) {
    fMusicFadingIn = TRUE;
    return (TRUE);
  }
  return (FALSE);
}

static void DoneFadeOutDueToEndMusic();

void MusicPoll() {
  int32_t iVol;

  SoundServiceStreams();
  SoundServiceRandom();

  // Handle Sound every sound overhead time....
  if (COUNTERDONE(MUSICOVERHEAD)) {
    // Reset counter
    RESETCOUNTER(MUSICOVERHEAD);

    if (fMusicFadingIn) {
      if (uiMusicHandle != NO_SAMPLE) {
        iVol = SoundGetVolume(uiMusicHandle);
        iVol = std::min((int32_t)uiMusicVolume, iVol + gbFadeSpeed);
        SoundSetVolume(uiMusicHandle, iVol);
        if (iVol == (int32_t)uiMusicVolume) {
          fMusicFadingIn = FALSE;
          gbFadeSpeed = 1;
        }
      }
    } else if (fMusicFadingOut) {
      if (uiMusicHandle != NO_SAMPLE) {
        iVol = SoundGetVolume(uiMusicHandle);
        iVol = (iVol >= 1) ? iVol - gbFadeSpeed : 0;

        iVol = std::max((int32_t)iVol, 0);

        SoundSetVolume(uiMusicHandle, iVol);
        if (iVol == 0) {
          MusicStop();
          fMusicFadingOut = FALSE;
          gbFadeSpeed = 1;
        }
      }
    }

    // #endif

    if (gfMusicEnded) {
      // OK, based on our music mode, play another!
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Music End Loop %d %d", uiMusicHandle, gubMusicMode));

      // If we were in victory mode, change!
      if (gbVictorySongCount == 1 || gbDeathSongCount == 1) {
        if (gbDeathSongCount == 1 && guiCurrentScreen == GAME_SCREEN) {
          CheckAndHandleUnloadingOfCurrentWorld();
        }

        if (gbVictorySongCount == 1) {
          SetMusicMode(MUSIC_TACTICAL_NOTHING);
        }
      } else {
        if (!gfDontRestartSong) {
          StartMusicBasedOnMode();
        }
      }

      gfMusicEnded = FALSE;
      gfDontRestartSong = FALSE;
    }
  }
}

void SetMusicMode(uint8_t ubMusicMode) {
  static int8_t bPreviousMode = 0;

  // OK, check if we want to restore
  if (ubMusicMode == MUSIC_RESTORE) {
    if (bPreviousMode == MUSIC_TACTICAL_VICTORY || bPreviousMode == MUSIC_TACTICAL_DEATH) {
      bPreviousMode = MUSIC_TACTICAL_NOTHING;
    }

    ubMusicMode = bPreviousMode;
  } else {
    // Save previous mode...
    bPreviousMode = gubOldMusicMode;
  }

  // if different, start a new music song
  if (gubOldMusicMode != ubMusicMode) {
    // Set mode....
    gubMusicMode = ubMusicMode;

    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Music New Mode %d %d", uiMusicHandle, gubMusicMode));

    gbVictorySongCount = 0;
    gbDeathSongCount = 0;

    if (uiMusicHandle != NO_SAMPLE) {
      // Fade out old music
      MusicFadeOut();
    } else {
      // Change music!
      StartMusicBasedOnMode();
    }
  }
  gubOldMusicMode = gubMusicMode;
}

static void StartMusicBasedOnMode() {
  static BOOLEAN fFirstTime = TRUE;

  if (fFirstTime) {
    fFirstTime = FALSE;

    bNothingModeSong = NOTHING_A_MUSIC + (int8_t)Random(4);

    bEnemyModeSong = TENSOR_A_MUSIC + (int8_t)Random(3);

    bBattleModeSong = BATTLE_A_MUSIC + (int8_t)Random(2);
  }

  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("StartMusicBasedOnMode() %d %d", uiMusicHandle, gubMusicMode));

  // Setup a song based on mode we're in!
  switch (gubMusicMode) {
    case MUSIC_MAIN_MENU:
      // ATE: Don't fade in
      gbFadeSpeed = (int8_t)uiMusicVolume;
      MusicPlay(MENUMIX_MUSIC);
      break;

    case MUSIC_LAPTOP:
      gbFadeSpeed = (int8_t)uiMusicVolume;
      MusicPlay(MARIMBAD2_MUSIC);
      break;

    case MUSIC_TACTICAL_NOTHING:
      // ATE: Don't fade in
      gbFadeSpeed = (int8_t)uiMusicVolume;
      if (gfUseCreatureMusic) {
        MusicPlay(CREEPY_MUSIC);
      } else {
        MusicPlay(bNothingModeSong);
        bNothingModeSong = NOTHING_A_MUSIC + (int8_t)Random(4);
      }
      break;

    case MUSIC_TACTICAL_ENEMYPRESENT:
      // ATE: Don't fade in EnemyPresent...
      gbFadeSpeed = (int8_t)uiMusicVolume;
      if (gfUseCreatureMusic) {
        MusicPlay(CREEPY_MUSIC);
      } else {
        MusicPlay(bEnemyModeSong);
        bEnemyModeSong = TENSOR_A_MUSIC + (int8_t)Random(3);
      }
      break;

    case MUSIC_TACTICAL_BATTLE:
      // ATE: Don't fade in
      gbFadeSpeed = (int8_t)uiMusicVolume;
      if (gfUseCreatureMusic) {
        MusicPlay(CREATURE_BATTLE_MUSIC);
      } else {
        MusicPlay(bBattleModeSong);
      }
      bBattleModeSong = BATTLE_A_MUSIC + (int8_t)Random(2);
      break;

    case MUSIC_TACTICAL_VICTORY:

      // ATE: Don't fade in EnemyPresent...
      gbFadeSpeed = (int8_t)uiMusicVolume;
      MusicPlay(TRIUMPH_MUSIC);
      gbVictorySongCount++;

      if (gfUseCreatureMusic && !gbWorldSectorZ) {  // We just killed all the creatures that just
                                                    // attacked the town.
        gfUseCreatureMusic = FALSE;
      }
      break;

    case MUSIC_TACTICAL_DEATH:

      // ATE: Don't fade in EnemyPresent...
      gbFadeSpeed = (int8_t)uiMusicVolume;
      MusicPlay(DEATH_MUSIC);
      gbDeathSongCount++;
      break;

    default:
      MusicFadeOut();
      break;
  }
}

static void MusicStopCallback(void *pData) {
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Music EndCallback %d %d", uiMusicHandle, gubMusicMode));

  gfMusicEnded = TRUE;
  uiMusicHandle = NO_SAMPLE;
}

void SetMusicFadeSpeed(int8_t bFadeSpeed) { gbFadeSpeed = bFadeSpeed; }

static void DoneFadeOutDueToEndMusic() {
  // Quit game....
  InternalLeaveTacticalScreen(MAINMENU_SCREEN);
  // SetPendingNewScreen( MAINMENU_SCREEN );
}
