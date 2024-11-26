#ifndef __TYPES_
#define __TYPES_

#include <stdint.h>

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

template <typename T>
static inline void Swap(T &a, T &b) {
  T t(a);
  a = b;
  b = t;
}

typedef unsigned char BOOLEAN;
typedef char STRING512[512];

typedef char SGPFILENAME[100];

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
  int32_t iLeft;
  int32_t iTop;
  int32_t iRight;
  int32_t iBottom;

  void set(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    iLeft = left;
    iTop = top;
    iRight = right;
    iBottom = bottom;
  }
};

struct SGPPoint {
  int32_t iX;
  int32_t iY;

  void set(int32_t x, int32_t y) {
    iX = x;
    iY = y;
  }
};

struct SDL_Color;
typedef SDL_Color SGPPaletteEntry;

typedef uint32_t COLORVAL;

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

#define SGP_TRANSPARENT ((uint16_t)0)

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
