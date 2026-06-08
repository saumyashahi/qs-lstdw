#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "sign.h"
#include "../common/hash.h"
#include "../lattice/polyvec.h"
#include "../../config/params.h"

/*
 * Compute Fiat-Shamir challenge  c_j = Hc(pk_j, m_j, w_j)
 *
 * Raccoon serialization: 7 bytes per coefficient (49-bit q).
 * w_agg_rounded contains rounded values which are small after >> NU_W,
 * but we keep 7 bytes for uniformity and future-proofing.
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
    /* 7 bytes per coefficient of w_agg_rounded */
    size_t w_bytes_len = (size_t)(QS_K * QS_N * 7);
    uint8_t *w_bytes = (uint8_t *)__builtin_alloca(w_bytes_len);

    size_t pos = 0;
    for (int i = 0; i < QS_K; i++) {
        for (int j = 0; j < QS_N; j++) {
            int64_t x = w_agg_rounded->vec[i].coeffs[j];
            /* 7-byte little-endian */
            w_bytes[pos++] = (uint8_t)( x        & 0xFF);
            w_bytes[pos++] = (uint8_t)((x >>  8) & 0xFF);
            w_bytes[pos++] = (uint8_t)((x >> 16) & 0xFF);
            w_bytes[pos++] = (uint8_t)((x >> 24) & 0xFF);
            w_bytes[pos++] = (uint8_t)((x >> 32) & 0xFF);
            w_bytes[pos++] = (uint8_t)((x >> 40) & 0xFF);
            w_bytes[pos++] = (uint8_t)((x >> 48) & 0xFF);
        }
    }

    Hc(c, pk_bytes, pk_bytes_len, msg, msglen, w_bytes, w_bytes_len);
}
