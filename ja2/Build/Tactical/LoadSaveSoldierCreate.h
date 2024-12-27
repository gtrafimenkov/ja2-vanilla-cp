// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVESOLIDERCREATE_H
#define LOADSAVESOLIDERCREATE_H

#include "Tactical/SoldierCreate.h"

uint16_t CalcSoldierCreateCheckSum(const SOLDIERCREATE_STRUCT *const s);

void ExtractSoldierCreateFromFile(HWFILE, SOLDIERCREATE_STRUCT *, bool stracLinuxFormat);

/**
 * Load SOLDIERCREATE_STRUCT structure and checksum from the file and guess the
 * format the structure was saved in (vanilla windows format or stracciatella
 * linux format). */
void ExtractSoldierCreateFromFileWithChecksumAndGuess(HWFILE, SOLDIERCREATE_STRUCT *,
                                                      uint16_t *checksum);

void InjectSoldierCreateIntoFile(HWFILE, SOLDIERCREATE_STRUCT const *);

#endif
