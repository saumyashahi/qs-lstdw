#ifndef QS_MATRIX_H
#define QS_MATRIX_H

#include "params.h"
#include "poly.h"

// Ensure `params.h` is included first to define `K` and `L`.
// Ensure all dependencies are included for types like `poly_t`, `K`, and `L`.
// Ensure matrix_t is defined only once.

/* A is K x L matrix of polynomials */
typedef struct {
    poly_t entries[QS_K][QS_L];
} matrix_t;

/* Vector of length L */
typedef struct {
    poly_t vec[QS_L];
} polyvec_l_t;

/* Vector of length K */
typedef struct {
    poly_t vec[QS_K];
} polyvec_k_t;

/* r = A * s */

void matrix_vec_mul(polyvec_k_t *r,
                        const matrix_t *A,
                        const polyvec_l_t *s);

/* Expand matrix A using seed */
void matrix_expand(matrix_t *A, const uint8_t seed[32]);

void matrix_ntt(matrix_t *A);

#endif