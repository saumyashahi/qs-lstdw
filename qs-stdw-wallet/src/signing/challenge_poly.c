#include "challenge_poly.h"
#include "../common/hash.h"
#include "../../config/params.h"
#include <string.h>

/*
 * Expand a 32-byte Fiat-Shamir challenge hash into a ternary challenge
 * polynomial c in R_q with exactly OMEGA = 19 nonzero ±1 coefficients.
 *
 * Raccoon uses omega = 19 (not 60 like Dilithium).
 * Polynomial degree n = 512 (not 256).
 * Position sampling requires 2 bytes per trial (since n=512 > 255).
 */
void qs_expand_challenge(poly_t *c_poly, const uint8_t challenge_hash[32])
{
    for (int i = 0; i < QS_N; i++)
        c_poly->coeffs[i] = 0;

    /*
     * Stream bytes from SHAKE256(challenge_hash).
     * First 8 bytes: 64 sign bits (we need OMEGA=19 sign bits).
     * Remaining: pairs of bytes for position sampling in [0, 511].
     */
    uint8_t buf[8 + OMEGA * 4];   /* generous: 4 bytes per attempt */
    shake256(buf, sizeof(buf), challenge_hash, 32);

    uint64_t signs = 0;
    for (int i = 0; i < 8; i++)
        signs |= ((uint64_t)buf[i]) << (8 * i);

    int placed   = 0;
    int buf_pos  = 8;

    while (placed < OMEGA) {
        if (buf_pos + 2 > (int)sizeof(buf)) {
            /* Re-hash (very rare) */
            uint8_t next_in[33];
            memcpy(next_in, challenge_hash, 32);
            next_in[32] = (uint8_t)placed;
            shake256(buf, sizeof(buf), next_in, 33);
            buf_pos = 0;
        }

        /* 2-byte little-endian position in [0, 65535], reject if >= 512 */
        uint16_t raw = (uint16_t)buf[buf_pos] | ((uint16_t)buf[buf_pos+1] << 8);
        buf_pos += 2;

        int pos = (int)(raw % QS_N);   /* uniform mod 512 — negligible bias */

        if (c_poly->coeffs[pos] != 0)
            continue;   /* already taken */

        /* +1 or -1 depending on next sign bit */
        int64_t sign = (signs & 1) ? (int64_t)(RACCOON_Q - 1) : 1LL;
        signs >>= 1;

        c_poly->coeffs[pos] = sign;
        placed++;
    }
}
