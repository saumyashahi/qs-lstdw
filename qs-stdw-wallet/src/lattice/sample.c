#include "poly.h"
#include "../common/prng.h"
#include "../../config/params.h"

/*
 * Sample a ternary secret polynomial with coefficients in {-1, 0, 1}.
 *
 * Raccoon uses ternary {-1, 0, +1} secrets (not bounded uniform ETA).
 * We map each 2-bit value:
 *   00 -> 0
 *   01 -> 1
 *   10 -> q-1  (represents -1 mod q)
 *   11 -> reject (re-sample)
 *
 * This gives a uniform distribution over {-1, 0, 1}.
 */
void sample_small_poly(poly_t *p, qs_prng_t *prng)
{
    for (int i = 0; i < QS_N; i++) {
        uint8_t byte;
        uint8_t bits;
        do {
            prng_squeeze(prng, &byte, 1);
            bits = byte & 0x03;
        } while (bits == 3);

        if (bits == 0)
            p->coeffs[i] = 0;
        else if (bits == 1)
            p->coeffs[i] = 1;
        else
            p->coeffs[i] = RACCOON_Q - 1;   /* -1 mod q */
    }
}
