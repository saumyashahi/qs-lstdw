#include "sign.h"
#include "../lattice/polyvec.h"
#include "../../config/params.h"

/*
 * round_{nu_w} applied to polyvec_k — thin wrapper over polyvec_k_round_nuw.
 * nu_w = 44 for Raccoon-128.
 */
void qs_round_nuw(polyvec_k_t *out, const polyvec_k_t *in)
{
    polyvec_k_round_nuw(out, in);
}

/*
 * Serialise polyvec_k to bytes: 7 bytes per coefficient (little-endian).
 * Used to feed t'_pk into the challenge hash Hc.
 */
void polyvec_k_to_bytes(uint8_t *out, size_t outlen, const polyvec_k_t *pk)
{
    size_t pos = 0;
    for (int i = 0; i < QS_K && pos + 7 <= outlen; i++) {
        for (int j = 0; j < QS_N && pos + 7 <= outlen; j++) {
            int64_t x = pk->vec[i].coeffs[j];
            out[pos++] = (uint8_t)( x        & 0xFF);
            out[pos++] = (uint8_t)((x >>  8) & 0xFF);
            out[pos++] = (uint8_t)((x >> 16) & 0xFF);
            out[pos++] = (uint8_t)((x >> 24) & 0xFF);
            out[pos++] = (uint8_t)((x >> 32) & 0xFF);
            out[pos++] = (uint8_t)((x >> 40) & 0xFF);
            out[pos++] = (uint8_t)((x >> 48) & 0xFF);
        }
    }
}
