#ifndef QS_POLY_H
#define QS_POLY_H

#include <stdint.h>
#include "../../config/params.h"

/*
 * Polynomial in Z_q[x]/(x^n+1), n=512.
 * Coefficients are int64_t because Raccoon's q = ~2^49 does not fit int32_t.
 * All arithmetic is done mod q = RACCOON_Q.
 */
typedef struct {
    int64_t coeffs[QS_N];
} poly_t;

void poly_add(poly_t *r, const poly_t *a, const poly_t *b);
void poly_sub(poly_t *r, const poly_t *a, const poly_t *b);
void poly_mul(poly_t *r, const poly_t *a, const poly_t *b);
void poly_zero(poly_t *a);
void poly_copy(poly_t *r, const poly_t *a);
void poly_reduce(poly_t *a);
int64_t poly_norm_inf(const poly_t *a);
int64_t poly_norm(const poly_t *a);

#endif /* QS_POLY_H */