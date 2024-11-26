#ifndef _MUSIC_CONTROL_H_
#define _MUSIC_CONTROL_H_

#include "SGP/SoundMan.h"

enum MusicList {
  MARIMBAD2_MUSIC,
  MENUMIX_MUSIC,
  NOTHING_A_MUSIC,
  NOTHING_B_MUSIC,
  NOTHING_C_MUSIC,
  NOTHING_D_MUSIC,
  TENSOR_A_MUSIC,
  TENSOR_B_MUSIC,
  TENSOR_C_MUSIC,
  TRIUMPH_MUSIC,
  DEATH_MUSIC,
  BATTLE_A_MUSIC,
  BATTLE_B_MUSIC,  // same as tensor B
  CREEPY_MUSIC,
  CREATURE_BATTLE_MUSIC,
  NUM_MUSIC
};

extern const char *const szMusicList[NUM_MUSIC];

enum MusicMode {

  MUSIC_NONE,
  MUSIC_RESTORE,
  MUSIC_MAIN_MENU,
  MUSIC_TACTICAL_NOTHING,
  MUSIC_TACTICAL_ENEMYPRESENT,
  MUSIC_TACTICAL_BATTLE,
  MUSIC_TACTICAL_VICTORY,
  MUSIC_TACTICAL_DEATH,
  MUSIC_LAPTOP,
};

extern uint8_t gubMusicMode;
extern BOOLEAN gfForceMusicToTense;

void SetMusicMode(uint8_t ubMusicMode);

/* Starts up one of the tunes in the music list. */
void MusicPlay(uint32_t uiNum);

void MusicSetVolume(uint32_t uiVolume);
uint32_t MusicGetVolume();

/* Handles any maintenance the music system needs done. Should be polled from
 * the main loop, or somewhere with a high frequency of calls. */
void MusicPoll();

void SetMusicFadeSpeed(int8_t bFadeSpeed);

#endif
