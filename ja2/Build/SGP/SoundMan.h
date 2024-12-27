// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SOUNDMAN_
#define __SOUNDMAN_

#include "SGP/Types.h"

#define MAXVOLUME 127

// Sound error values (they're all the same)
#define NO_SAMPLE 0xffffffff
#define SOUND_ERROR 0xffffffff

// Zeros out the structs for the system info, and initializes the cache.
void InitializeSoundManager();

/* Silences all currently playing sound, deallocates any memory allocated, and
 * releases the sound hardware. */
void ShutdownSoundManager();

uint32_t SoundPlayFromBuffer(int16_t *pbuffer, uint32_t size, uint32_t volume, uint32_t pan,
                             uint32_t loop, void (*end_callback)(void *), void *data);

/* Starts a sample playing. If the sample is not loaded in the cache, it will
 * be found and loaded.
 *
 * Returns: If the sound was started, it returns a sound ID unique to that
 *          instance of the sound If an error occured, SOUND_ERROR will be
 *          returned
 *
 * !!Note:  Can no longer play streamed files */
uint32_t SoundPlay(const char *pFilename, uint32_t volume, uint32_t pan, uint32_t loop,
                   void (*end_callback)(void *), void *data);

/* The sample will be played as a double-buffered sample.
 *
 * Returns: If the sound was started, it returns a sound ID unique to that
 *          instance of the sound If an error occured, SOUND_ERROR will be
 *          returned */
uint32_t SoundPlayStreamedFile(const char *pFilename, uint32_t volume, uint32_t pan, uint32_t loop,
                               void (*end_callback)(void *), void *data);

/* Registers a sample to be played randomly within the specified parameters.
 *
 * * Samples designated "random" are ALWAYS loaded into the cache, and locked
 * in place. They are never double-buffered, and this call will fail if they
 * cannot be loaded. *
 *
 * Returns: If successful, it returns the sample index it is loaded to, else
 *          SOUND_ERROR is returned. */
uint32_t SoundPlayRandom(const char *pFilename, uint32_t time_min, uint32_t time_max,
                         uint32_t vol_min, uint32_t vol_max, uint32_t pan_min, uint32_t pan_max,
                         uint32_t max_instances);

/* Can be polled in tight loops where sound buffers might starve due to heavy
 * hardware use, etc. Streams DO NOT normally need to be serviced manually, but
 * in some cases (heavy file loading) it might be desirable.
 * If you are using the end of sample callbacks, you must call this function
 * periodically to check the sample's status. */
void SoundServiceStreams();

/* This function should be polled by the application if random samples are
 * used. The time marks on each are checked and if it is time to spawn a new
 * instance of the sound, the number already in existance are checked, and if
 * there is room, a new one is made and the count updated.
 * If random samples are not being used, there is no purpose in polling this
 * function. */
void SoundServiceRandom();

// Stops all currently playing sounds.
void SoundStopAll();

void SoundStopAllRandom();

/* Stops the playing of a sound instance, if still playing.
 *
 * Returns: TRUE if the sample was actually stopped, FALSE if it could not be
 *          found, or was not playing. */
BOOLEAN SoundStop(uint32_t uiSoundID);

void SoundStopRandom(uint32_t uiSample);

// Returns TRUE/FALSE that an instance of a sound is still playing.
BOOLEAN SoundIsPlaying(uint32_t uiSoundID);

/* Sets the volume on a currently playing sound.
 *
 * Returns: TRUE if the volume was actually set on the sample, FALSE if the
 *          sample had already expired or couldn't be found */
BOOLEAN SoundSetVolume(uint32_t uiSoundID, uint32_t uiVolume);

/* Sets the pan on a currently playing sound.
 *
 * Returns: TRUE if the pan was actually set on the sample, FALSE if the sample
 *          had already expired or couldn't be found */
BOOLEAN SoundSetPan(uint32_t uiSoundID, uint32_t uiPan);

/* Returns the current volume setting of a sound that is playing. If the sound
 * has expired, or could not be found, SOUND_ERROR is returned. */
uint32_t SoundGetVolume(uint32_t uiSoundID);

/* Reports the current time position of the sample.
 * Note: You should be checking SoundIsPlaying very carefully while calling
 * this function.
 *
 * Returns: The current time of the sample in milliseconds. */
uint32_t SoundGetPosition(uint32_t uiSoundID);

// Allows or disallows the startup of the sound hardware.
void SoundEnableSound(BOOLEAN fEnable);

#endif
