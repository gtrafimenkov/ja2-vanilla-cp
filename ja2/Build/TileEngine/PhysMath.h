// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __PHYS_MATH_H
#define __PHYS_MATH_H

#include "SGP/Types.h"

typedef float real;

struct vector_3 {
  real x, y, z;
};

vector_3 VAdd(vector_3 *a, vector_3 *b);
vector_3 VMultScalar(vector_3 *a, real b);
real VDotProduct(vector_3 *a, vector_3 *b);
vector_3 VGetNormal(vector_3 *a);

#endif
