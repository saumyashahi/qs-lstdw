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
    master_public_key_t mpk;
    master_secret_key_t msk;

    /* 1. Generate master key */
    master_keygen(&mpk, &msk);

    /* 2. Prepare PRNG for Shamir randomness */
    qs_prng_t prng;
    uint8_t seed[32];
    memset(seed, 0xAA, 32);  /* deterministic test seed */
    prng_init(&prng, seed);

    /* 3. Split secret */
    shamir_share_t shares[NUM_SHARES];

    shamir_split(&msk.s,
                 shares,
                 NUM_SHARES,
                 THRESHOLD,
                 &prng);

    /* 4. Reconstruct using first THRESHOLD shares */
    polyvec_l_t reconstructed;

    shamir_reconstruct(&reconstructed,
                       shares,
                       THRESHOLD);

    /* 5. Compare */
    if (compare_polyvec(&msk.s, &reconstructed)) {
        printf("Shamir reconstruction SUCCESS\n");
    } else {
        printf("Shamir reconstruction FAILED\n");

        /* Print first mismatch for debugging */
        for (int i = 0; i < QS_L; i++) {
            for (int k = 0; k < QS_N; k++) {
                if (msk.s.vec[i].coeffs[k] !=
                    reconstructed.vec[i].coeffs[k]) {

                    printf("Mismatch at vec %d coeff %d:\n", i, k);
                    printf("Original: %u\n",
                           msk.s.vec[i].coeffs[k]);
                    printf("Reconstructed: %u\n",
                           reconstructed.vec[i].coeffs[k]);
                    return 1;
                }
            }
        }
    }

    return 0;
}