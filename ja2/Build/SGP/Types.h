#ifndef __TYPES_
#define __TYPES_

#include <stdint.h>
#include <stdlib.h>

#include "SGP/Platform.h"
#include "SGP/SGPStrings.h"

#define UNIMPLEMENTED                                                                     \
  fprintf(stderr, "===> %s:%d: %s() is not implemented\n", __FILE__, __LINE__, __func__); \
  abort();

#ifdef WITH_FIXMES
#define FIXME fprintf(stderr, "===> %s:%d: %s() FIXME\n", __FILE__, __LINE__, __func__);
#else
#define FIXME (void)0;
#endif

#define lengthof(a) (sizeof(a) / sizeof(a[0]))
#define endof(a) ((a) + lengthof(a))

#define MAX(a, b) __max(a, b)
#define MIN(a, b) __min(a, b)

#define FOR_EACHX(type, iter, array, x) \
  for (type *iter = (array); iter != endof((array)); (x), ++iter)
#define FOR_EACH(type, iter, array) FOR_EACHX(type, iter, (array), (void)0)

template <typename T>
static inline void Swap(T &a, T &b) {
  T t(a);
  a = b;
  b = t;
}

typedef int32_t INT;
typedef int32_t INT32;
typedef uint32_t UINT;
typedef uint32_t UINT32;

// integers
typedef uint8_t UINT8;
typedef int8_t INT8;
typedef uint16_t UINT16;
typedef int16_t INT16;
// floats
typedef float FLOAT;
typedef double DOUBLE;
// strings
typedef char CHAR8;

// other
typedef unsigned char BOOLEAN;
typedef void *PTR;
typedef UINT8 BYTE;
typedef CHAR8 STRING512[512];

#define SGPFILENAME_LEN 100
typedef CHAR8 SGPFILENAME[SGPFILENAME_LEN];

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define BAD_INDEX -1

#define PI 3.1415926

#ifndef NULL
#define NULL 0
#endif

struct SGPBox {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;

  void set(int32_t _x, int32_t _y, int32_t _w, int32_t _h) {
    x = _x;
    y = _y;
    w = _w;
    h = _h;
  }
};

struct SGPRect {
  INT32 iLeft;
  INT32 iTop;
  INT32 iRight;
  INT32 iBottom;

  void set(INT32 left, INT32 top, INT32 right, INT32 bottom) {
    iLeft = left;
    iTop = top;
    iRight = right;
    iBottom = bottom;
  }
};

struct SGPPoint {
  INT32 iX;
  INT32 iY;

  void set(INT32 x, INT32 y) {
    iX = x;
    iY = y;
  }
};

struct SDL_Color;
typedef SDL_Color SGPPaletteEntry;

typedef UINT32 COLORVAL;

struct AuxObjectData;
struct ETRLEObject;
struct RelTileLoc;
struct SGPImage;

class SGPVObject;
typedef SGPVObject *HVOBJECT;
typedef SGPVObject *Font;

class SGPVSurface;

struct BUTTON_PICS;

struct SGPFile;
typedef SGPFile *HWFILE;

#define SGP_TRANSPARENT ((UINT16)0)

#ifdef __cplusplus
#define ENUM_BITSET(type)                                                                \
  static inline type operator~(type a) { return (type) ~(int)a; }                        \
  static inline type operator&(type a, type b) { return (type)((int)a & (int)b); }       \
  static inline type operator&=(type &a, type b) { return a = (type)((int)a & (int)b); } \
  static inline type operator|(type a, type b) { return (type)((int)a | (int)b); }       \
  static inline type operator|=(type &a, type b) { return a = (type)((int)a | (int)b); }
#else
#define ENUM_BITSET(type)
#endif

#endif
