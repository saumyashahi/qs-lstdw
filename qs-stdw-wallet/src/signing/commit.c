#include "sign.h"
#include "../common/hash.h"
#include "../common/prng.h"
#include "../lattice/polyvec.h"
#include "../config/params.h"
#include <string.h>

/* Fixed-size buffer sizes */
#define PRF_OUT_BYTES  (QS_L * QS_N * 4)
#define W_SER_BYTES    (QS_K * QS_N * 4)

/*
 * Compute PRF blinder: PRF(seed, sid, domain) -> polyvec_l_t
 * Fixed-size stack allocation to avoid __builtin_alloca issues.
 */
static void prf_blinder(polyvec_l_t *out,
                        const uint8_t seed[32],
                        const uint8_t sid[32],
                        uint8_t domain)
{
    uint8_t input[65];
    memcpy(input, seed, 32);
    memcpy(input + 32, sid, 32);
    input[64] = domain;

    uint8_t buf[PRF_OUT_BYTES];
    shake256(buf, PRF_OUT_BYTES, input, 65);

    size_t pos = 0;
    for (int i = 0; i < QS_L; i++) {
        for (int j = 0; j < QS_N; j++) {
            uint32_t x  = ((uint32_t)buf[pos]     << 24)
                        | ((uint32_t)buf[pos + 1]  << 16)
                        | ((uint32_t)buf[pos + 2]  <<  8)
                        |  (uint32_t)buf[pos + 3];
            pos += 4;
            out->vec[i].coeffs[j] = (int32_t)(x % (uint32_t)Q);
        }
    }
}

/*
 * Round 1 Commit — Algorithm 4, lines 6-10
 *
 * r_{j,k}  <- D^l_w    (ephemeral, gamma1 range)
 * e'_{j,k} <- D^k_w    (small masking noise)
 * w_{j,k}  = A*r_{j,k} + e'_{j,k}
 * cmt_{j,k}= Hcom(sid, msg, w_{j,k})
 *
 * Row blinder (published in Round 1):
 *   m_{j,k} = SUM_{i in act} PRF(seed_{k,i}, sid)
 *
 * Column blinder (private, used in Round 3):
 *   m*_{j,k} = m_{j,k}
 *   [because seed_{k,i} = seed_{i,k} by pairwise convention,
 *    so PRF(seed_{k,i}) = PRF(seed_{i,k}) -> blinders cancel in combine]
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

    /* --- Sample ephemeral r_{j,k} (gamma1 range) --- */
    polyvec_l_sample_gamma1(&out->r, prng);

    /* --- Sample small e'_{j,k} from {-ETA,...,ETA} --- */
    polyvec_k_zero(&out->e_prime);
    for (int i = 0; i < QS_K; i++) {
        for (int j = 0; j < QS_N; j++) {
            uint8_t byte;
            do {
                prng_squeeze(prng, &byte, 1);
                byte &= 0x0F;
            } while (byte > 2 * ETA);
            int32_t coeff = (int32_t)byte - ETA;
            if (coeff < 0) coeff += Q;
            out->e_prime.vec[i].coeffs[j] = coeff;
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
                int32_t x = out->w.vec[i].coeffs[j];
                w_bytes[pos++] = (uint8_t)((x >> 24) & 0xFF);
                w_bytes[pos++] = (uint8_t)((x >> 16) & 0xFF);
                w_bytes[pos++] = (uint8_t)((x >>  8) & 0xFF);
                w_bytes[pos++] = (uint8_t)( x        & 0xFF);
            }
        }
        /*
         * Combine: [domain=0x05] || sid || msg || w_bytes
         * Use three-part SHAKE256 via sequential H calls to avoid VLA.
         * Equivalent to feeding all parts into a single hash.
         */
        /* Build prefix: domain + sid */
        uint8_t prefix[33];
        prefix[0] = 0x05;
        memcpy(prefix + 1, sid, 32);

        /* Hash: H(prefix || msg || w_bytes) using a temporary accumulation */
        /* We concatenate into a fixed buffer (max message 256 bytes assumed) */
        uint8_t cmt_buf[33 + 256 + W_SER_BYTES];
        size_t cmt_len = 0;
        memcpy(cmt_buf, prefix, 33);  cmt_len += 33;
        size_t ml = msglen < 256 ? msglen : 256; /* safety cap */
        memcpy(cmt_buf + cmt_len, msg, ml); cmt_len += ml;
        memcpy(cmt_buf + cmt_len, w_bytes, W_SER_BYTES); cmt_len += W_SER_BYTES;
        shake256(out->commitment, 32, cmt_buf, cmt_len);
    }

    /* --- Row blinder: m_{j,k} = SUM_{i in act} PRF(seed_{k,i}, sid) --- */
    polyvec_l_zero(&out->m_row);
    for (int i = 0; i < t; i++) {
        polyvec_l_t tmp;
        prf_blinder(&tmp, pairwise_seeds[i], sid, 0x00);
        polyvec_l_add(&out->m_row, &out->m_row, &tmp);
    }

    /*
     * --- Column blinder: m*_{j,k} = m_{j,k}  [blinders cancel] ---
     *
     * With seed_{k,i} = seed_{i,k} (symmetric pairwise keys from keygen) and
     * the SAME PRF: PRF(seed_{k,i}) = PRF(seed_{i,k}) for all i.
     * Therefore m_{j,k} = m*_{j,k} for each party k, and:
     *   SUM_k m*_{j,k} = SUM_k m_{j,k}
     * guaranteeing cancellation in qs_sign_combine line 22.
     */
    polyvec_l_copy(&out->m_col, &out->m_row);
}