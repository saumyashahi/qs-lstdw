#include <stdio.h>
#include <string.h>

#include "../src/keygen/master_keygen.h"
#include "../src/threshold/shamir.h"
#include "../src/threshold/share.h"
#include "../src/common/prng.h"

#define NUM_SHARES 5
#define THRESHOLD  3

static int compare_polyvec(const polyvec_l_t *a,
                           const polyvec_l_t *b)
{
    for (int i = 0; i < QS_L; i++) {
        for (int k = 0; k < QS_N; k++) {
            if (a->vec[i].coeffs[k] != b->vec[i].coeffs[k])
                return 0;
        }
    }
    return 1;
}

int main(void)
{
    /* 1. Generate a random secret to split */
    qs_prng_t prng;
    uint8_t seed[32];
    memset(seed, 0xAA, 32);  /* deterministic test seed */
    prng_init(&prng, seed);

    polyvec_l_t secret;
    polyvec_l_uniform(&secret, &prng);

    /* 2. Split secret */
    shamir_share_t shares[NUM_SHARES];

    shamir_split(&secret,
                 shares,
                 NUM_SHARES,
                 THRESHOLD,
                 &prng);

    /* 3. Reconstruct using first THRESHOLD shares */
    polyvec_l_t reconstructed;

    shamir_reconstruct(&reconstructed,
                       shares,
                       THRESHOLD);

    printf("\n=========================================================\n");
    printf("   SHAMIR SECRET SHARING TEST\n");
    printf("=========================================================\n");

    /* 4. Compare */
    if (compare_polyvec(&secret, &reconstructed)) {
        printf("[PASS] Shamir reconstruction SUCCESS\n");
    } else {
        printf("[FAIL] ERROR: Shamir reconstruction FAILED\n");

        /* Print first mismatch for debugging */
        for (int i = 0; i < QS_L; i++) {
            for (int k = 0; k < QS_N; k++) {
                if (secret.vec[i].coeffs[k] !=
                    reconstructed.vec[i].coeffs[k]) {

                    printf("[FAIL] Mismatch at vec %d coeff %d:\n", i, k);
                    printf("[INFO] Original: %u\n",
                           secret.vec[i].coeffs[k]);
                    printf("[INFO] Reconstructed: %u\n",
                           reconstructed.vec[i].coeffs[k]);
                    return 1;
                }
            }
        }
    }

    printf("=========================================================\n");
    return 0;
}