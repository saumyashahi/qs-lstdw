#include "ntt.h"
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
 * Matrix-vector multiplication: r = A * s
 *
 * Accumulate the dot product in NTT domain; apply one invNTT per row.
 * A must already be in NTT domain (matrix_ntt called after matrix_expand).
 * s is converted to NTT domain here.
 */
void matrix_vec_mul(polyvec_k_t *r, const matrix_t *A, const polyvec_l_t *s)
{
    polyvec_l_t s_ntt;
    polyvec_l_copy(&s_ntt, s);
    polyvec_l_ntt(&s_ntt);

    for (int i = 0; i < QS_K; i++) {
        poly_zero(&r->vec[i]);
        for (int j = 0; j < QS_L; j++) {
            poly_t tmp;
            poly_pointwise_mul(&tmp, &A->entries[i][j], &s_ntt.vec[j]);
            poly_add(&r->vec[i], &r->vec[i], &tmp);
        }
        /* Single invNTT per row AFTER accumulating the full dot product */
        poly_invntt(&r->vec[i]);
    }
}

void matrix_ntt(matrix_t *A)
{
    for (int i = 0; i < QS_K; i++)
        for (int j = 0; j < QS_L; j++)
            poly_ntt(&A->entries[i][j]);
}
