#ifndef __MACRO_H
#define __MACRO_H

#if !defined(_WIN32)
/* Not Visual Studio, not MINGW */
#define __max(a, b) ((a) > (b) ? (a) : (b))
#define __min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define MAX(a, b) __max(a, b)
#define MIN(a, b) __min(a, b)

#define UNIMPLEMENTED                                                                     \
  fprintf(stderr, "===> %s:%d: %s() is not implemented\n", __FILE__, __LINE__, __func__); \
  abort();

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#endif
