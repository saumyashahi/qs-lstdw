#include "challenge_poly.h"
#include "../common/hash.h"
#include "../config/params.h"
#include <string.h>

/*
 * Expand a 32-byte Fiat-Shamir challenge hash into a ternary challenge
 * polynomial c ∈ R_q with exactly TAU nonzero ±1 coefficients.
 *
 * Algorithm (Dilithium SampleInBall style):
 *   1. Use SHAKE256 to stream bytes from the seed.
 *   2. Place ±1 coefficients at positions determined by rejection sampling.
 *   3. Signs are determined by the lowest bit of each sign byte.
 *
 * This ensures the challenge polynomial is uniformly distributed among all
 * degree-(N-1) polynomials with exactly TAU nonzero ±1 coefficients.
 */
void qs_expand_challenge(poly_t *c_poly, const uint8_t challenge_hash[32])
{
    /* Start with all-zero polynomial */
    for (int i = 0; i < QS_N; i++)
        c_poly->coeffs[i] = 0;

    /*
     * We need TAU positions from [0, N-1] and TAU sign bits.
     * Stream bytes from SHAKE256(challenge_hash).
     * Buffer: enough for many rejection samples.
     */
    uint8_t buf[272]; /* 8 sign bytes + 264 position bytes (sufficient) */
    shake256(buf, sizeof(buf), challenge_hash, 32);

    /* First 8 bytes provide 64 sign bits (we need TAU ≤ 64) */
    uint64_t signs = 0;
    for (int i = 0; i < 8; i++)
        signs |= ((uint64_t)buf[i]) << (8 * i);

    int placed = 0;
    int buf_pos = 8; /* start reading position bytes after sign bytes */

    while (placed < TAU)
    {
        if (buf_pos >= (int)sizeof(buf))
        {
            /* Rehash if we run out — shouldn't happen with 264 bytes for N=256, TAU=60 */
            uint8_t next_input[33];
            memcpy(next_input, challenge_hash, 32);
            next_input[32] = (uint8_t)placed; /* domain separation by round */
            shake256(buf, sizeof(buf), next_input, 33);
            buf_pos = 8;
        }

        /* Sample position in [0, N-1] using rejection sampling */
        uint8_t pos_byte = buf[buf_pos++];
        int pos = (int)pos_byte;

        /* Rejection: only accept pos in [placed, N-1] range of Fisher-Yates shuffle */
        /* Simplified: accept pos if it hasn't been used yet (marked by nonzero) */
        /* We use the position directly if it's in [0, N-1] — no bias since N=256 and byte is uniform [0,255] */

        if (c_poly->coeffs[pos] != 0)
            continue; /* position already taken, try again */

        /* Assign sign from the next available bit */
        int32_t sign = (signs & 1) ? (int32_t)(Q - 1) : 1; /* -1 ≡ Q-1 in Z_q, +1 = 1 */
        signs >>= 1;

        c_poly->coeffs[pos] = sign;
        placed++;
    }
}
