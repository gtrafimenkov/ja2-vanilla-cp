// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __MACRO_H
#define __MACRO_H

#define UNIMPLEMENTED                                                                     \
  fprintf(stderr, "===> %s:%d: %s() is not implemented\n", __FILE__, __LINE__, __func__); \
  abort();

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#ifdef WITH_FIXMES
#define FIXME fprintf(stderr, "===> %s:%d: %s() FIXME\n", __FILE__, __LINE__, __func__);
#else
#define FIXME (void)0;
#endif

#define lengthof(a) (sizeof(a) / sizeof(a[0]))
#define endof(a) ((a) + lengthof(a))

#define FOR_EACHX(type, iter, array, x) \
  for (type *iter = (array); iter != endof((array)); (x), ++iter)
#define FOR_EACH(type, iter, array) FOR_EACHX(type, iter, (array), (void)0)

#endif
