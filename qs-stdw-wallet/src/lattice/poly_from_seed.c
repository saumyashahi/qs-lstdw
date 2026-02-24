#include <stdint.h>
#include <string.h>
#include "../common/hash.h"
#include "poly_from_seed.h"

/* DecodeSmall: map bytes to small integers in [-ETA, ETA] */

static int32_t decode_small(uint8_t byte)
{
#if USE_BOUNDED_SAMPLING
    return (byte % (2*ETA + 1)) - ETA;
#else
    /* Placeholder for Gaussian sampling */
    return (byte % (2*ETA + 1)) - ETA;
#endif
}

static void i2osp(uint16_t i, uint8_t out[2])
{
    out[0] = (i >> 8) & 0xFF;
    out[1] = i & 0xFF;
}

void poly_from_seed(int32_t a[QS_N], const uint8_t seed[SEED_BYTES])
{
    uint8_t input[SEED_BYTES + 2];
    uint8_t output[1];

    for (uint16_t i = 0; i < QS_N; i++) {

        memcpy(input, seed, SEED_BYTES);
        i2osp(i, input + SEED_BYTES);

        shake256(output, 1, input, sizeof(input));

        int32_t val = decode_small(output[0]);

        if (val < 0)
            val += Q;

        a[i] = val;
    }
}