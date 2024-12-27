// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#pragma once

#include "SGP/Platform.h"

/* Platform and compiler-specifics */

/**************************************************************
 * Input/Output
 *************************************************************/

#if defined(_MSC_VER)
#include <direct.h>
#include <io.h>
#define chdir(path) _chdir(path)
#define chmod(path, access_mode) _chmod(path, access_mode)
#define close(file_handle) _close(file_handle)
#define fdopen(file_handle, format) _fdopen(file_handle, format)
#define fileno(file) _fileno(file)
#define open(filename, flags) _open(filename, flags)
#define open3(filename, flags, permission) _open(filename, flags, permission)
#define unlink(path) _unlink(path)
#else
#include <unistd.h>
#define open3(filename, flags, permission) open(filename, flags, permission)

/* #  if defined __APPLE__ && defined __MACH__ */
/* #    include <CoreFoundation/CoreFoundation.h> */
/* #    include <sys/param.h> */
/* #  endif */

#endif

#ifdef _WIN32
#define mkdir(path, mode) mkdir(path)
#endif

/**************************************************************
 *
 *************************************************************/
