#include "matrix.h"
#include "poly_from_seed.h"
#include <string.h>

void matrix_expand(matrix_t *A, const uint8_t seed[32])
{
    for (int i = 0; i < QS_K; i++) {
        for (int j = 0; j < QS_L; j++) {

            uint8_t ext[34];
            memcpy(ext, seed, 32);
            ext[32] = (uint8_t)i;
            ext[33] = (uint8_t)j;

            poly_from_seed(A->entries[i][j].coeffs, ext);
        }
    }
}

/*
 * Matrix-vector multiplication:
 *
 * r_i = sum_j A[i][j] * s[j]
 */
void matrix_vec_mul(polyvec_k_t *r,
                    const matrix_t *A,
                    const polyvec_l_t *s)
{
    for (int i = 0; i < QS_K; i++) {

        /* Initialize r_i to zero */
        for (int c = 0; c < QS_N; c++)
            poly_zero(&r->vec[i]);

        for (int j = 0; j < QS_L; j++) {

            poly_t tmp;
            poly_mul(&tmp, &A->entries[i][j], &s->vec[j]);

            poly_add(&r->vec[i], &r->vec[i], &tmp);
        }
    }
}