// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#pragma once

/* Platform and compiler-specifics */

/**************************************************************
 * File system case-sensitivity
 *************************************************************/

#ifdef _WIN32
#define CASE_SENSITIVE_FS 0
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#define CASE_SENSITIVE_FS 1
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

/* #if CASE_SENSITIVE_FS */
/* #include <dirent.h> */
/* #endif */

/**************************************************************
 * Warnings
 *************************************************************/

/* Disable some warnings on Visual Studio */
#if defined(_MSC_VER)
#pragma warning(disable : 4018) /* example: '>' : signed/unsigned mismatch */
#pragma warning(disable : 4200) /* example: nonstandard extension used : \
                                   zero-sized array in struct/union */
#pragma warning(disable : 4244) /* example: conversion from 'int' to 'real', \
                                   possible loss of data */
#pragma warning(disable : 4800) /* example: 'const uint32_t' : forcing value to bool 'true' \
                                   or 'false' (performance warning) */
#pragma warning(disable : 4996) /* example: 'wcscpy': This function or variable may be  \
                                   unsafe. Consider using wcscpy_s instead. To disable  \
                                   deprecation, use _CRT_SECURE_NO_WARNINGS. See online \
                                   help for details. */
#endif

/**************************************************************
 * Misc
 *************************************************************/

#if defined(_MSC_VER)
/* Visual Studio */
#define __func__ __FUNCTION__
#endif

/**************************************************************
 *
 *************************************************************/
