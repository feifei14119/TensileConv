#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include "ff_basic.h"

namespace feifei
{
#define	MIN_FP32_ERR		(1e-6)

#define MAX_16BIT_UINT		(65535)

#define next2pow(n)	do{ int base = 1; \
	for (int i = 0; i < 32; i++) { base = 1 << i; if (n <= base) { break; }} \
	return base; } while (0)

#define _log2(value)	do{ int log2 = 0; \
	while (value > 1) { value = value / 2; log2++; } \
	return log2; } while(0)

#define isPow2(value)  ((value & (value - 1)) == 0)

#define modMask(value) (value - 1)

#define divCeil(a,b)	((a + b - 1) / b)

#define randInt10(a,b) ((rand() % (b-a)) + a)   	// [a,b)
#define randInt11(a,b) ((rand() % (b-a+1)) + a) 	// [a,b]
#define randInt01(a,b) ((rand() % (b-a)) + a + 1)	// (a,b]
}