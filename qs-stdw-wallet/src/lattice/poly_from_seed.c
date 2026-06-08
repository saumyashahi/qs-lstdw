#include <stdint.h>
#include <string.h>
#include "../common/hash.h"
#include "poly_from_seed.h"
#include "../../config/params.h"

/*
 * POLYFROMSEED for Raccoon-128.
 *
 * Expands a 32-byte seed into a polynomial in R_q with coefficients
 * uniformly distributed in [0, q-1] via 7-byte rejection sampling.
 *
 * A single SHAKE256 call produces all bytes needed, avoiding the
 * per-coefficient overhead of the old implementation.
 *
 * Byte budget: each coefficient needs 7 bytes (56 bits) for a 49-bit q.
 * Worst-case rejection rate is tiny (~2^{-7}), so 512*7 = 3584 bytes
 * is enough in practice.  We generate 4096 bytes and retry from the
 * remainder if needed (extremely rare).
 */

static void i2osp2(uint16_t i, uint8_t out[2])
{
    out[0] = (i >> 8) & 0xFF;
    out[1] =  i       & 0xFF;
}

void poly_from_seed(int64_t a[QS_N], const uint8_t seed[SEED_BYTES])
{
    /* Extend seed with domain tag to match paper's PolyFromSeed call */
    uint8_t input[SEED_BYTES + 2];
    memcpy(input, seed, SEED_BYTES);
    i2osp2(0, input + SEED_BYTES);   /* index 0 = "full polynomial" */

    /* Generous buffer: 7 bytes per coeff, 512 coeffs = 3584 needed */
    const size_t BUF = 4096;
    uint8_t buf[4096];
    shake256(buf, BUF, input, sizeof(input));

    int filled = 0;
    size_t pos  = 0;

    while (filled < QS_N) {
        if (pos + 7 > BUF) {
            /* Re-expand with incremented counter (very rare) */
            i2osp2((uint16_t)filled, input + SEED_BYTES);
            shake256(buf, BUF, input, sizeof(input));
            pos = 0;
        }
        uint64_t val = (uint64_t)buf[pos]
                     | ((uint64_t)buf[pos+1] <<  8)
                     | ((uint64_t)buf[pos+2] << 16)
                     | ((uint64_t)buf[pos+3] << 24)
                     | ((uint64_t)buf[pos+4] << 32)
                     | ((uint64_t)buf[pos+5] << 40)
                     | ((uint64_t)buf[pos+6] << 48);
        pos += 7;
        val &= 0x0001FFFFFFFFFFFFULL;   /* 49-bit mask */
        if ((int64_t)val < RACCOON_Q) {
            a[filled++] = (int64_t)val;
        }
    }
}
