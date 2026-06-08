#include "sign.h"
#include "../common/hash.h"
#include "../common/prng.h"
#include "../lattice/polyvec.h"
#include "../../config/params.h"
#include <string.h>

/* Serialized size of polyvec_k in bytes (7 bytes per coeff for 49-bit q) */
#define W_SER_BYTES  (QS_K * QS_N * 7)
/* PRF output size for one blinder polyvec_l */
#define PRF_OUT_BYTES (QS_L * QS_N * 7)

/*
 * Compute PRF blinder: PRF(seed, sid, domain) -> polyvec_l_t
 * Uses SHAKE256(seed || sid || domain) and 7-byte rejection sampling.
 */
static void prf_blinder(polyvec_l_t *out,
                        const uint8_t seed[32],
                        const uint8_t sid[32],
                        uint8_t domain)
{
    uint8_t input[65];
    memcpy(input,      seed, 32);
    memcpy(input + 32, sid,  32);
    input[64] = domain;

    /* Generate enough bytes for QS_L * QS_N coefficients at 7 bytes each */
    uint8_t buf[PRF_OUT_BYTES + 512];  /* extra for rare rejections */
    shake256(buf, sizeof(buf), input, 65);

    size_t pos = 0;
    for (int i = 0; i < QS_L; i++) {
        for (int j = 0; j < QS_N; j++) {
            uint64_t val;
            do {
                if (pos + 7 > sizeof(buf)) {
                    /* Re-hash — extremely rare */
                    input[64]++;
                    shake256(buf, sizeof(buf), input, 65);
                    pos = 0;
                }
                val = (uint64_t)buf[pos]
                    | ((uint64_t)buf[pos+1] <<  8)
                    | ((uint64_t)buf[pos+2] << 16)
                    | ((uint64_t)buf[pos+3] << 24)
                    | ((uint64_t)buf[pos+4] << 32)
                    | ((uint64_t)buf[pos+5] << 40)
                    | ((uint64_t)buf[pos+6] << 48);
                pos += 7;
                val &= 0x0001FFFFFFFFFFFFULL;
            } while ((int64_t)val >= RACCOON_Q);
            out->vec[i].coeffs[j] = (int64_t)val;
        }
    }
}

/*
 * Round 1 Commit — Algorithm 4, lines 6-10
 *
 * r_{j,k}  ~ uniform in [-B_w, B_w]   (B_w = 2^nu_w)
 * e'_{j,k} ~ ternary {-1,0,+1}        (Raccoon masking noise)
 * w_{j,k}  = A*r_{j,k} + e'_{j,k}
 * cmt      = Hcom(sid, msg, w_{j,k})
 * m_{j,k}  = SUM_{i in act} PRF(seed_{k,i}, sid)
 */
void qs_sign_commit(
    qs_commit_share *out,
    const uint8_t sid[32],
    const uint8_t *msg,
    size_t msglen,
    const matrix_t *A,
    const uint8_t pairwise_seeds[][32],
    const int *active_set,
    int t,
    int signer_idx,
    qs_prng_t *prng
)
{
    (void)signer_idx;
    (void)active_set;

    /* --- Sample ephemeral r_{j,k} ~ uniform in [-B_w, B_w] --- */
    polyvec_l_sample_raccoon(&out->r, prng);

    /* --- Sample masking noise e'_{j,k} ternary {-1,0,+1} --- */
    polyvec_k_zero(&out->e_prime);
    for (int i = 0; i < QS_K; i++) {
        for (int j = 0; j < QS_N; j++) {
            uint8_t byte, bits;
            do {
                prng_squeeze(prng, &byte, 1);
                bits = byte & 0x03;
            } while (bits == 3);

            if (bits == 0)
                out->e_prime.vec[i].coeffs[j] = 0;
            else if (bits == 1)
                out->e_prime.vec[i].coeffs[j] = 1;
            else
                out->e_prime.vec[i].coeffs[j] = RACCOON_Q - 1; /* -1 mod q */
        }
    }

    /* --- w_{j,k} = A*r + e' --- */
    matrix_vec_mul(&out->w, A, &out->r);
    polyvec_k_add(&out->w, &out->w, &out->e_prime);

    /* --- cmt_{j,k} = Hcom(sid, msg, w_{j,k}) --- */
    {
        uint8_t w_bytes[W_SER_BYTES];
        size_t pos = 0;
        for (int i = 0; i < QS_K; i++) {
            for (int j = 0; j < QS_N; j++) {
                int64_t x = out->w.vec[i].coeffs[j];
                w_bytes[pos++] = (uint8_t)( x        & 0xFF);
                w_bytes[pos++] = (uint8_t)((x >>  8) & 0xFF);
                w_bytes[pos++] = (uint8_t)((x >> 16) & 0xFF);
                w_bytes[pos++] = (uint8_t)((x >> 24) & 0xFF);
                w_bytes[pos++] = (uint8_t)((x >> 32) & 0xFF);
                w_bytes[pos++] = (uint8_t)((x >> 40) & 0xFF);
                w_bytes[pos++] = (uint8_t)((x >> 48) & 0xFF);
            }
        }
        /* Hcom(domain || sid || msg || w_bytes) */
        uint8_t prefix[33];
        prefix[0] = 0x05;
        memcpy(prefix + 1, sid, 32);

        size_t ml = msglen < 256 ? msglen : 256;
        size_t total = 33 + ml + W_SER_BYTES;
        uint8_t *cmt_buf = (uint8_t *)__builtin_alloca(total);
        memcpy(cmt_buf,          prefix,   33);
        memcpy(cmt_buf + 33,     msg,      ml);
        memcpy(cmt_buf + 33 + ml, w_bytes, W_SER_BYTES);
        shake256(out->commitment, 32, cmt_buf, total);
    }

    /* --- Row blinder: m_{j,k} = SUM_{i in act} PRF(seed_{k,i}, sid) --- */
    polyvec_l_zero(&out->m_row);
    for (int i = 0; i < t; i++) {
        polyvec_l_t tmp;
        prf_blinder(&tmp, pairwise_seeds[i], sid, 0x00);
        polyvec_l_add(&out->m_row, &out->m_row, &tmp);
    }

    /* Column blinder = row blinder (blinders cancel by pairwise symmetry) */
    polyvec_l_copy(&out->m_col, &out->m_row);
}
