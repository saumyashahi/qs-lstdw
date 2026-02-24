#include "matrix.h"

/*
 * Matrix-vector multiplication:
 *
 * r_i = sum_j A[i][j] * s[j]
 */
void matrix_vec_mul(polyvec_k_t *r,
                    const matrix_t *A,
                    const polyvec_l_t *s)
{
    for (int i = 0; i < K; i++) {

        /* Initialize r_i to zero */
        for (int c = 0; c < N; c++)
            r->vec[i].coeffs[c] = 0;

        for (int j = 0; j < L; j++) {

            poly_t tmp;
            poly_mul(&tmp, &A->entries[i][j], &s->vec[j]);

            poly_add(&r->vec[i], &r->vec[i], &tmp);
        }
    }
}