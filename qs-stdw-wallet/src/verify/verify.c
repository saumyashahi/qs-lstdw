#include <string.h>
#include <stdio.h>

#include "verify.h"
#include "../signing/sign.h"
#include "../signing/challenge_poly.h"
#include "../common/hash.h"
#include "../lattice/matrix.h"
#include "../lattice/polyvec.h"
#include "../config/params.h"


/*
 * Algorithm 5 — QS-STDW.Verify
 *
 * Input : pk_j = (A, t'_{pk_j}), message m, ctr, signature σ_j = (c_j, z_j, h_j)
 * Output: 1 if valid, 0 otherwise
 *
 * Steps:
 *   1. sid ← HSessionID(m || ctr)                             [handled by caller]
 *   2. pk_j ← RandPK(mpk, ch, sid)                           [handled by caller]
 *   3. Let pk_j = (A, t'_{pk_j})
 *   4. w'_j ← h_j + round_{ν_w}(A·z_j − 2^{ν_t}·c_j·t'_{pk_j})
 *   5. c'_j ← Hc(pk_j, m_j, w'_j)
 *   6. if c'_j ≠ c_j  → return 0
 *   7. if ||z_j||_∞ ≥ β_z  or  ||w'_j||_∞ ≥ β_w  → return 0
 *   8. HintCheck: verify h_j = w_j − y_j is consistent → return 0 on fail
 *   9. return 1
 */

int qs_verify(
    const uint8_t challenge[32],   /* c_j: 32-byte Fiat-Shamir challenge from signature */
    const polyvec_l_t *z,          /* z_j */
    const polyvec_k_t *h,          /* h_j */
    const polyvec_k_t *t_pk,       /* t'_{pk_j}: rerandomized public key trapdoor */
    const uint8_t *pk_bytes,       /* serialized t'_pk for Hc */
    size_t pk_bytes_len,
    const uint8_t *msg,
    size_t msglen,
    const matrix_t *A
)
{
    /* ----------------------------------------------------------------
     * Step 4: Recompute w'_j = h_j + round_{ν_w}(A·z_j - 2^{ν_t}·c_j·t'_pk_j)
     * ---------------------------------------------------------------- */

    /* 4a. Az = A * z_j */
    polyvec_k_t Az;
    matrix_vec_mul(&Az, A, z);

    /* 4b. Expand challenge hash → challenge polynomial c_poly */
    poly_t c_poly;
    qs_expand_challenge(&c_poly, challenge);

    /* 4c. ct = c_poly * t'_pk_j  (challenge polynomial × public key trapdoor) */
    polyvec_k_t ct;
    polyvec_k_mul_poly(&ct, &c_poly, t_pk);

    /* 4d. scaled = 2^{ν_t} * ct */
    polyvec_k_t scaled;
    polyvec_k_shift_left(&scaled, &ct, NU_T);

    /* 4e. v = Az - scaled */
    polyvec_k_t v;
    polyvec_k_sub(&v, &Az, &scaled);

    /* 4f. y = round_{ν_w}(v) */
    polyvec_k_t y;
    polyvec_k_round_nuw(&y, &v);

    /* 4g. w' = h + y */
    polyvec_k_t w_prime;
    polyvec_k_add(&w_prime, h, &y);

    /* ----------------------------------------------------------------
     * Step 5: Recompute challenge c'_j = Hc(pk_j, m_j, w'_j)
     * ---------------------------------------------------------------- */
    uint8_t challenge_check[32];
    qs_compute_challenge(
        challenge_check,
        &w_prime,
        pk_bytes,
        pk_bytes_len,
        msg,
        msglen
    );

    /* ----------------------------------------------------------------
     * Step 6: Challenge equality check
     * ---------------------------------------------------------------- */
    if (memcmp(challenge, challenge_check, 32) != 0) {
        return 0;
    }

    /* ----------------------------------------------------------------
     * Step 7: Norm bounds
     *   ||z_j||_∞ < β_z  (GAMMA1 - ETA)
     *   ||w'_j||_∞ < β_w  (GAMMA2)
     * ---------------------------------------------------------------- */
    int32_t z_norm   = polyvec_l_norm_inf(z);
    int32_t w_norm   = polyvec_k_norm_inf(&w_prime);

    if (z_norm >= BETA_Z) {
        return 0;
    }
    if (w_norm >= BETA_W) {
        return 0;
    }

    /* ----------------------------------------------------------------
     * Step 8: HintCheck — verify that h_j = w_j' - y is consistent
     *   i.e. h_j coefficients are small (within expected range)
     *   Simplified check: ||h_j||_∞ < β_w
     * ---------------------------------------------------------------- */
    int32_t h_norm = polyvec_k_norm_inf(h);
    if (h_norm >= BETA_W) {
        return 0;
    }

    /* ----------------------------------------------------------------
     * Step 9: Accept
     * ---------------------------------------------------------------- */
    return 1;
}