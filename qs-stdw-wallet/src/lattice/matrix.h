#ifndef QS_MATRIX_H
#define QS_MATRIX_H

#include "poly.h"

/* A is K x L matrix of polynomials */
typedef struct {
    poly_t entries[K][L];
} matrix_t;

/* Vector of length L */
typedef struct {
    poly_t vec[L];
} polyvec_l_t;

/* Vector of length K */
typedef struct {
    poly_t vec[K];
} polyvec_k_t;

/* r = A * s */
void matrix_vec_mul(polyvec_k_t *r,
                    const matrix_t *A,
                    const polyvec_l_t *s);

#endif