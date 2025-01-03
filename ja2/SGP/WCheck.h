// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __WCHECK_
#define __WCHECK_

#if 0  // XXX TODO put under some flag
#define FAIL abort();
#else
#define FAIL FIXME
#endif

#define CHECKF(exp)      \
  do {                   \
    if (!(exp)) {        \
      FAIL return FALSE; \
    }                    \
  } while (0)
#define CHECKV(exp) \
  do {              \
    if (!(exp)) {   \
      FAIL return;  \
    }               \
  } while (0)
#define CHECKN(exp)     \
  do {                  \
    if (!(exp)) {       \
      FAIL return NULL; \
    }                   \
  } while (0)

#endif
