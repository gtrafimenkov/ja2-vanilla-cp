#ifndef ENCRYPTED_FILE_H
#define ENCRYPTED_FILE_H

#include "SGP/Types.h"

void LoadEncryptedData(HWFILE, wchar_t *DestString, uint32_t seek_chars, uint32_t read_chars);
void LoadEncryptedDataFromFile(char const *Filename, wchar_t DestString[], uint32_t seek_chars,
                               uint32_t read_chars);

#endif
