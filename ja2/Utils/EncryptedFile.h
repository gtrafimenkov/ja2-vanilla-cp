// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef ENCRYPTED_FILE_H
#define ENCRYPTED_FILE_H

#include "SGP/Types.h"

void LoadEncryptedData(HWFILE, wchar_t *DestString, uint32_t seek_chars, uint32_t read_chars);
void LoadEncryptedDataFromFile(char const *Filename, wchar_t DestString[], uint32_t seek_chars,
                               uint32_t read_chars);

#endif
