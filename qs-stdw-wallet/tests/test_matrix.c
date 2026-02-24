#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "matrix.h"
#include "poly.h"
#include "sample.h"

int main() {

    uint8_t seed[32] = {1};

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

    qs_prng_t prng1, prng2;
    prng_init(&prng1, seed);
    prng_init(&prng2, seed);

    poly_t a, b;

    sample_small_poly(&a, &prng1);
    sample_small_poly(&b, &prng2);

    if (memcmp(&a, &b, sizeof(poly_t)) != 0) {
        printf("Sampler not deterministic\n");
        return 1;
    }

    printf("Sampler test passed.\n");
    return 0;
}