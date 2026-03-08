#include "sign.h"
#include "../lattice/polyvec.h"
#include "../config/params.h"

/*
 * round_{ν_w}  applied to a polyvec_k
 *
 * Algorithm 4 line 15 and 23:
 *   w_j  = round_{ν_w}(Σ_{k∈act} w_{j,k})
 *   y_j  = round_{ν_w}(...)
 *
 * Convention (matching polyvec.c::polyvec_k_round_nuw):
 *   out[i] = centered(in[i]) >> NU_W
 *
 * This function is a thin wrapper kept for backward compatibility.
 * The core implementation lives in polyvec_k_round_nuw().
 */
void qs_round_nuw(polyvec_k_t *out, const polyvec_k_t *in)
{
    polyvec_k_round_nuw(out, in);
}

/*
 * Serialise polyvec_k to bytes (big-endian 4 bytes per coefficient).
 * Used to feed t'_pk into the challenge hash Hc.
 */
void polyvec_k_to_bytes(uint8_t *out, size_t outlen, const polyvec_k_t *pk)
{
    size_t pos = 0;
    for (int i = 0; i < QS_K && pos + 4 <= outlen; i++) {
        for (int j = 0; j < QS_N && pos + 4 <= outlen; j++) {
            int32_t x = pk->vec[i].coeffs[j];
            out[pos++] = (uint8_t)((x >> 24) & 0xFF);
            out[pos++] = (uint8_t)((x >> 16) & 0xFF);
            out[pos++] = (uint8_t)((x >>  8) & 0xFF);
            out[pos++] = (uint8_t)( x        & 0xFF);
        }
    }
}