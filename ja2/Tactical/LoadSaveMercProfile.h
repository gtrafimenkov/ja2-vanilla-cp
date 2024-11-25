// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEMERCPROFILE_H
#define LOADSAVEMERCPROFILE_H

#include "SGP/IEncodingCorrector.h"
#include "Tactical/SoldierProfileType.h"

#define MERC_PROFILE_SIZE (716)             /**< Vanilla Merc profile size */
#define MERC_PROFILE_SIZE_STRAC_LINUX (796) /**< Stracciatella Linux Merc profile size */

/**
 * Extract merc profile from the binary data.
 * @param encodingCorrection Perform encoding correction - it is necessary for
 * loading strings from the game data files. */
void ExtractMercProfile(uint8_t const *const Src, MERCPROFILESTRUCT &p, bool stracLinuxFormat,
                        uint32_t *checksum, const IEncodingCorrector *fixer);

/** Calculates soldier profile checksum. */
uint32_t SoldierProfileChecksum(MERCPROFILESTRUCT const &p);

/** Extract IMP merc profile from file.
 * If saved checksum is not correct, exception will be thrown. */
void ExtractImpProfileFromFile(const char *fileName, int32_t *iProfileId, int32_t *iPortraitNumber,
                               MERCPROFILESTRUCT &p);

void InjectMercProfile(uint8_t *Dst, MERCPROFILESTRUCT const &);
void InjectMercProfileIntoFile(HWFILE, MERCPROFILESTRUCT const &);

/** Load raw merc profiles.
 * @param f Open file with profile data.
 * @param numProfiles Number of profiles to load
 * @param profiles Array for storing profile data */
void LoadRawMercProfiles(HWFILE const f, int numProfiles, MERCPROFILESTRUCT *profiles,
                         const IEncodingCorrector *fixer);

#endif
