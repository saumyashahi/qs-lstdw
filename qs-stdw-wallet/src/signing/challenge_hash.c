#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "sign.h"
#include "../common/hash.h"
#include "../lattice/polyvec.h"

/*
 * Compute Fiat-Shamir challenge
 *
 * Algorithm 4, line 16:
 *   c_j = Hc(pk_j, m_j, w_j)
 *
 * where:
 *   pk_j = (A, t'_{pk_j})  — the rerandomized session public key
 *   m_j  = message
 *   w_j  = round_{ν_w}(Σ_{k∈act} w_{j,k})  — aggregated rounded commitment
 *
 * The Hc function uses domain separation (prefix byte 0x02) to distinguish
 * it from the plain H function. This is defined in common/hash.c.
 *
 * We serialize:
 *   pk_bytes: serialization of t'_{pk_j}  (A is fixed/public, no need to include)
 *   w_bytes : 4 bytes per coefficient of w_agg_rounded
 */
void qs_compute_challenge(
    uint8_t c[32],
    const polyvec_k_t *w_agg_rounded,
    const uint8_t *pk_bytes,
    size_t pk_bytes_len,
    const uint8_t *msg,
    size_t msglen
)
{
    /* Serialize w_agg_rounded: QS_K * QS_N * 4 bytes (big-endian) */
    size_t w_bytes_len = (size_t)(QS_K * QS_N * 4);
    uint8_t w_bytes[QS_K * QS_N * 4];

    size_t pos = 0;
    for (int i = 0; i < QS_K; i++) {
        for (int j = 0; j < QS_N; j++) {
            int32_t x = w_agg_rounded->vec[i].coeffs[j];
            w_bytes[pos++] = (uint8_t)((x >> 24) & 0xFF);
            w_bytes[pos++] = (uint8_t)((x >> 16) & 0xFF);
            w_bytes[pos++] = (uint8_t)((x >>  8) & 0xFF);
            w_bytes[pos++] = (uint8_t)( x        & 0xFF);
        }
    }

    /* Hc(pk_bytes, msg, w_bytes) from hash.c */
    Hc(c, pk_bytes, pk_bytes_len, msg, msglen, w_bytes, w_bytes_len);
}