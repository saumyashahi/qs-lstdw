#ifndef QS_POLY_H
#define QS_POLY_H

#include <stdint.h>
#include "../../config/params.h"

typedef struct {
    int32_t coeffs[QS_N];
} poly_t;

/* Basic operations */

void poly_add(poly_t *r, const poly_t *a, const poly_t *b);
void poly_sub(poly_t *r, const poly_t *a, const poly_t *b);
void poly_mul(poly_t *r, const poly_t *a, const poly_t *b);

/* Utilities */

void poly_copy(poly_t *r, const poly_t *a);
void poly_reduce(poly_t *a);
int32_t poly_norm_inf(const poly_t *a);

#endif