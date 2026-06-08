#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../config/params.h"

/* Updated signature: int64_t for Raccoon 49-bit q */
void poly_from_seed(int64_t a[QS_N], const uint8_t seed[SEED_BYTES]);

int main(void)
{
    printf("\n=========================================================\n");
    printf("   POLY TEST (Raccoon-128)\n");
    printf("=========================================================\n");

    uint8_t seed[SEED_BYTES] = {0};
    int64_t poly1[QS_N];
    int64_t poly2[QS_N];

    poly_from_seed(poly1, seed);
    poly_from_seed(poly2, seed);

    for (int i = 0; i < QS_N; i++) {
        if (poly1[i] != poly2[i]) {
            printf("[FAIL] Mismatch at index %d\n", i);
            return 1;
        }
        if (poly1[i] < 0 || poly1[i] >= RACCOON_Q) {
            printf("[FAIL] Coefficient out of range at index %d: %lld\n",
                   i, (long long)poly1[i]);
            return 1;
        }
    }

    printf("[PASS] Deterministic polynomial generation OK (n=%d, q=%lld)\n",
           QS_N, (long long)RACCOON_Q);
    printf("=========================================================\n");
    return 0;
}
