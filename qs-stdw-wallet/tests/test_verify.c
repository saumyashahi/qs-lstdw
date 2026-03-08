/*
 * test_verify.c — Dedicated verification test for QS-STDW
 *
 * Same polynomial-consistency fix as test_sign.c:
 * f_j is generated as a degree-(T-1) polynomial with ETA-bounded coefficients,
 * evaluated at 0 for t'_pk and at k for each party's rerandomized share.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "params.h"

#include "../lattice/matrix.h"
#include "../lattice/polyvec.h"
#include "../lattice/sample.h"

#include "../common/prng.h"
#include "../common/hash.h"

#include "../threshold/shamir.h"
#include "../threshold/share.h"

#include "../signing/sign.h"
#include "../signing/challenge_poly.h"

#include "../verify/verify.h"

#define N_SIGNERS 3
#define T_SIGNERS 2

#define PK_BYTES ((size_t)(QS_K * QS_N * 4))

/* Evaluate f_j(x) = Σ coeffs[d] * x^d at integer x_val */
static void eval_rerand_poly(polyvec_l_t *out,
                              const polyvec_l_t *coeffs,
                              int T,
                              int x_val)
{
    polyvec_l_zero(out);
    int64_t power = 1;
    for (int d = 0; d < T; d++) {
        polyvec_l_t term;
        polyvec_l_copy(&term, &coeffs[d]);
        polyvec_l_mul_scalar(&term, (int32_t)(power % (int64_t)Q));
        polyvec_l_add(out, out, &term);
        power = power * x_val;
    }
}

static void sign_message(
    const matrix_t *A,
    const polyvec_k_t *t_master,
    party_secret_t *parties,
    const uint8_t chaincode[CHAINCODE_BYTES],
    const uint8_t *msg, size_t msglen,
    uint32_t ctr,
    const int *active_set,
    qs_prng_t *prng,
    uint8_t challenge_out[32],
    polyvec_l_t *z_out,
    polyvec_k_t *h_out,
    polyvec_k_t *t_pk_out,
    uint8_t pk_bytes_out[]
)
{
    /* Session ID */
    uint8_t sid[SESSION_ID_BYTES];
    HSessionID(sid, msg, msglen, ctr);

    /* rho = H(chaincode || sid) */
    uint8_t rho_buf[CHAINCODE_BYTES + SESSION_ID_BYTES];
    memcpy(rho_buf, chaincode, CHAINCODE_BYTES);
    memcpy(rho_buf + CHAINCODE_BYTES, sid, SESSION_ID_BYTES);
    uint8_t rho[32];
    H(rho, rho_buf, sizeof(rho_buf));

    /* Generate f_j polynomial with small (ETA-bounded) coefficients */
    polyvec_l_t f_coeffs[T_SIGNERS];
    {
        qs_prng_t fj_prng;
        prng_init(&fj_prng, rho);
        for (int d = 0; d < T_SIGNERS; d++)
            for (int i = 0; i < QS_L; i++)
                sample_small_poly(&f_coeffs[d].vec[i], &fj_prng);
    }

    /* RandPK: t'_pk = t_master + A * f_j(0) */
    polyvec_l_t fj0;
    eval_rerand_poly(&fj0, f_coeffs, T_SIGNERS, 0);
    polyvec_k_t Af0;
    matrix_vec_mul(&Af0, A, &fj0);
    polyvec_k_add(t_pk_out, t_master, &Af0);
    polyvec_k_to_bytes(pk_bytes_out, PK_BYTES, t_pk_out);

    /* RandSK: sk_{j,k} = s_k + f_j(k) */
    polyvec_l_t sk_session[T_SIGNERS];
    for (int idx = 0; idx < T_SIGNERS; idx++) {
        int k = active_set[idx];
        polyvec_l_copy(&sk_session[idx], &parties[k - 1].share);
        polyvec_l_t fk;
        eval_rerand_poly(&fk, f_coeffs, T_SIGNERS, k);
        polyvec_l_add(&sk_session[idx], &sk_session[idx], &fk);
    }

    /* Round 1: Commit */
    qs_commit_share commits[T_SIGNERS];
    for (int idx = 0; idx < T_SIGNERS; idx++) {
        int k = active_set[idx];
        uint8_t pws[T_SIGNERS][32];
        for (int j = 0; j < T_SIGNERS; j++)
            memcpy(pws[j], parties[k - 1].pairwise_seeds[active_set[j] - 1], 32);
        qs_sign_commit(&commits[idx], sid, msg, msglen, A,
                       (const uint8_t (*)[32])pws, active_set, T_SIGNERS, idx, prng);
    }

    /* Round 2: Aggregate + Challenge */
    polyvec_k_t w_sum;
    polyvec_k_zero(&w_sum);
    for (int idx = 0; idx < T_SIGNERS; idx++)
        polyvec_k_add(&w_sum, &w_sum, &commits[idx].w);
    polyvec_k_t w_agg_rounded;
    polyvec_k_round_nuw(&w_agg_rounded, &w_sum);
    qs_compute_challenge(challenge_out, &w_agg_rounded, pk_bytes_out, PK_BYTES, msg, msglen);

    poly_t c_poly;
    qs_expand_challenge(&c_poly, challenge_out);

    /* Round 3: Response */
    polyvec_l_t z_shares[T_SIGNERS];
    for (int idx = 0; idx < T_SIGNERS; idx++) {
        int32_t lambda = qs_lagrange_coeff_modq(active_set[idx], active_set, T_SIGNERS);
        qs_sign_response(&z_shares[idx], &commits[idx].r, &sk_session[idx],
                         &commits[idx].m_col, &c_poly, lambda);
    }

    /* Combine */
    polyvec_l_t m_rows[T_SIGNERS];
    for (int idx = 0; idx < T_SIGNERS; idx++)
        polyvec_l_copy(&m_rows[idx], &commits[idx].m_row);
    qs_sign_combine(z_out, h_out, z_shares, m_rows, T_SIGNERS,
                    A, t_pk_out, &c_poly, &w_agg_rounded);
}

int main(void)
{
    printf("QS-STDW Verification Tests\n");
    printf("==========================\n");

    int total = 0, passed = 0;

    qs_prng_t prng;
    uint8_t rseed[32]; memset(rseed, 0xDE, 32);
    prng_init(&prng, rseed);

    /* KGen */
    uint8_t seed_A[32];
    prng_squeeze(&prng, seed_A, 32);
    matrix_t A;
    matrix_expand(&A, seed_A);

    polyvec_l_t master_s;
    for (int i = 0; i < QS_L; i++) sample_small_poly(&master_s.vec[i], &prng);
    polyvec_k_t e;
    for (int i = 0; i < QS_K; i++) sample_small_poly(&e.vec[i], &prng);
    polyvec_k_t t_master;
    matrix_vec_mul(&t_master, &A, &master_s);
    polyvec_k_add(&t_master, &t_master, &e);

    party_secret_t parties[N_SIGNERS];
    shamir_share_t shares[N_SIGNERS];
    qs_prng_t sp; uint8_t ss[32]; memset(ss, 0x77, 32); prng_init(&sp, ss);
    shamir_split(&master_s, shares, N_SIGNERS, T_SIGNERS, &sp);
    for (int i = 0; i < N_SIGNERS; i++) {
        parties[i].id = (uint32_t)(i + 1);
        parties[i].share = shares[i].value;
        for (int k = 0; k < N_SIGNERS; k++) {
            uint8_t ps[32];
            int lo = (i < k) ? i : k, hi = (i < k) ? k : i;
            memset(ps, (uint8_t)(lo * 7 + hi * 13 + 1), 32);
            memcpy(parties[i].pairwise_seeds[k], ps, 32);
        }
    }
    uint8_t chaincode[CHAINCODE_BYTES]; memset(chaincode, 0xBB, CHAINCODE_BYTES);

    const uint8_t msg[] = "quantum-safe threshold wallet signing";
    size_t msglen = sizeof(msg) - 1;
    int active[] = {1, 2};

    uint8_t challenge[32];
    polyvec_l_t z;
    polyvec_k_t h, t_pk;
    uint8_t pk_bytes[PK_BYTES];

    sign_message(&A, &t_master, parties, chaincode, msg, msglen, 0,
                 active, &prng, challenge, &z, &h, &t_pk, pk_bytes);

    /* Test 1: Valid signature */
    total++;
    {
        int r = qs_verify(challenge, &z, &h, &t_pk, pk_bytes, PK_BYTES, msg, msglen, &A);
        printf(r ? "[PASS] Test 1: Valid signature accepted\n"
                 : "[FAIL] Test 1: Valid signature rejected\n");
        if (r) passed++;
    }

    /* Test 2: Tampered message */
    total++;
    {
        const uint8_t bad[] = "tampered!";
        int r = qs_verify(challenge, &z, &h, &t_pk, pk_bytes, PK_BYTES,
                          bad, sizeof(bad)-1, &A);
        printf(!r ? "[PASS] Test 2: Tampered message rejected\n"
                  : "[FAIL] Test 2: Tampered message accepted (security bug!)\n");
        if (!r) passed++;
    }

    /* Test 3: Tampered challenge */
    total++;
    {
        uint8_t bad_c[32]; memcpy(bad_c, challenge, 32); bad_c[0] ^= 0xFF;
        int r = qs_verify(bad_c, &z, &h, &t_pk, pk_bytes, PK_BYTES, msg, msglen, &A);
        printf(!r ? "[PASS] Test 3: Tampered challenge rejected\n"
                  : "[FAIL] Test 3: Tampered challenge accepted (security bug!)\n");
        if (!r) passed++;
    }

    /* Test 4: Tampered z */
    total++;
    {
        polyvec_l_t bad_z; polyvec_l_copy(&bad_z, &z);
        bad_z.vec[0].coeffs[7] ^= 1;
        int r = qs_verify(challenge, &bad_z, &h, &t_pk, pk_bytes, PK_BYTES, msg, msglen, &A);
        printf(!r ? "[PASS] Test 4: Tampered z rejected\n"
                  : "[FAIL] Test 4: Tampered z accepted (security bug!)\n");
        if (!r) passed++;
    }

    /* Test 5: Tampered h */
    total++;
    {
        polyvec_k_t bad_h; polyvec_k_copy(&bad_h, &h);
        bad_h.vec[0].coeffs[3] ^= 1;
        int r = qs_verify(challenge, &z, &bad_h, &t_pk, pk_bytes, PK_BYTES, msg, msglen, &A);
        printf(!r ? "[PASS] Test 5: Tampered h rejected\n"
                  : "[FAIL] Test 5: Tampered h accepted (security bug!)\n");
        if (!r) passed++;
    }

    /* Test 6: Wrong public key */
    total++;
    {
        polyvec_k_t wrong_tpk; polyvec_k_zero(&wrong_tpk);
        uint8_t wrong_pk_bytes[PK_BYTES];
        polyvec_k_to_bytes(wrong_pk_bytes, PK_BYTES, &wrong_tpk);
        int r = qs_verify(challenge, &z, &h, &wrong_tpk,
                          wrong_pk_bytes, PK_BYTES, msg, msglen, &A);
        printf(!r ? "[PASS] Test 6: Wrong public key rejected\n"
                  : "[FAIL] Test 6: Wrong public key accepted (security bug!)\n");
        if (!r) passed++;
    }

    printf("\n==========================\n");
    printf("Results: %d / %d tests PASSED\n", passed, total);
    return (passed == total) ? 0 : 1;
}
