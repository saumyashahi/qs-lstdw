#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "matrix.h"
#include "poly.h"

int main() {

    uint8_t seed[32] = {0};

    matrix_t A1, A2;
    matrix_expand(&A1, seed);
    matrix_expand(&A2, seed);

    // Check deterministic expansion
    if (memcmp(&A1, &A2, sizeof(matrix_t)) != 0) {
        printf("Matrix expansion not deterministic\n");
        return 1;
    }

    polyvec_l_t s;
    polyvec_k_t r;

    // zero vector test
    for (int j = 0; j < QS_L; j++)
        poly_zero(&s.vec[j]);

    matrix_vec_mul(&r, &A1, &s);

    for (int i = 0; i < QS_K; i++)
        if (poly_norm(&r.vec[i]) != 0) {
            printf("Zero vector multiplication failed\n");
            return 1;
        }

    printf("Matrix tests passed.\n");
    return 0;
}