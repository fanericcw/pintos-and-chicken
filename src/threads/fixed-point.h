#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>

#define F (1 << 14)

#define INT_TO_FP(n) (n * F)
#define FP_TO_INT_ZERO(x) (x / F)
#define FP_TO_INT_NEAREST(x) (x >= 0 ? ((x + F / 2) / F) : ((x - F / 2) / F))

#define FP_ADD(x, y) (x + y)
#define FP_SUB(x, y) (x - y)
#define FP_ADD_INT(x, n) (x + n * F)
#define FP_SUB_INT(x, n) (x - n * F)
#define FP_MULT(x, y) (((int64_t) x) * y / F)
#define FP_MULT_INT(x, n) (x * n)
#define FP_DIV(x, y) (((int64_t) x) * F / y)
#define FP_DIV_INT(x, n) (x / n)

#endif /* threads/fixed-point.h */
