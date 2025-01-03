// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef SGPSTRINGS_H
#define SGPSTRINGS_H

#include <stdlib.h>

/*
 * Replace the path and/or extension of a filename.
 * If path is not null, the path of filename is replaced by path, otherwise it
 * is kept.  The extension of filename is replaced by ext.
 */
void ReplacePath(char *buf, size_t size, char const *path, char const *filename, char const *ext);

#endif
