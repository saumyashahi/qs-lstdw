#include "poly.h"
#include "prng.h"

void sample_small_poly(poly_t *p, qs_prng_t *prng)
{
    uint8_t buf[QS_N];
    prng_squeeze(prng, buf, QS_N);

    for (int i = 0; i < QS_N; i++) {
        uint8_t r = buf[i] % 3;

        if (r == 0)
            p->coeffs[i] = -1;
        else if (r == 1)
            p->coeffs[i] = 0;
        else
            p->coeffs[i] = 1;
    }
}