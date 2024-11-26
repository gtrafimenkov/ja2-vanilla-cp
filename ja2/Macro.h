#ifndef __MACRO_H
#define __MACRO_H

#define UNIMPLEMENTED                                                                     \
  fprintf(stderr, "===> %s:%d: %s() is not implemented\n", __FILE__, __LINE__, __func__); \
  abort();

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#endif
