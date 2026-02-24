#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../config/params.h"

void poly_from_seed(int32_t a[QS_N], const uint8_t seed[SEED_BYTES]);

int main()
{
    uint8_t seed[SEED_BYTES] = {0};
    int32_t poly1[QS_N];
    int32_t poly2[QS_N];

    poly_from_seed(poly1, seed);
    poly_from_seed(poly2, seed);

    for (int i = 0; i < QS_N; i++) {
        if (poly1[i] != poly2[i]) {
            printf("Mismatch at %d\n", i);
            return 1;
        }
    }

    printf("Deterministic polynomial test passed.\n");
    return 0;
}