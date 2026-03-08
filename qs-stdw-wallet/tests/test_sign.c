/*
 * test_sign.c — End-to-end threshold signing and verification test
 *
 * Implements Algorithm 4 (QS-STDW.Sign) + Algorithm 5 (QS-STDW.Verify)
 *
 * Key fix: rerandomization polynomial f_j(x) of degree T-1 is generated ONCE
 * from rho with small (ETA-bounded) coefficients, then evaluated CONSISTENTLY:
 *   - f_j(0)  = constant term  → used in t'_pk = t + A·f_j(0)
 *   - f_j(k)  = evaluation at k → added to each party k's secret share
 * This ensures Σ_k λ_k · (s_k + f_j(k)) = s_master + f_j(0),
 * which is required for the verification equation A·z - c·t'_pk = A·r - c·e.
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
#include "../common/randombytes.h"

#include "../threshold/shamir.h"
#include "../threshold/share.h"
#include "../threshold/rerandomize.h"

#include "../signing/sign.h"
#include "../signing/challenge_poly.h"

#include "../verify/verify.h"

#define N_SIGNERS   3
#define T_SIGNERS   2

#define PK_BYTES ((size_t)(QS_K * QS_N * 4))

static void pk_to_bytes(uint8_t *out, const polyvec_k_t *pk)
{
    polyvec_k_to_bytes(out, PK_BYTES, pk);
}

/*
 * Evaluate the rerandomization polynomial f_j at an integer point:
 *   f_j(x) = Σ_{d=0}^{T-1} coeffs[d] · x^d
 *
 * Each coeffs[d] is a polyvec_l_t with small (ETA-bounded) coefficients.
 * Result is accumulated into `out` which must be pre-zeroed.
 */
static void eval_rerand_poly(polyvec_l_t *out,
                             const polyvec_l_t *coeffs,
                             int T,
                             int x_val)
{
    polyvec_l_zero(out);
    int64_t power = 1;  /* x^d */

    for (int d = 0; d < T; d++) {
        polyvec_l_t term;
        polyvec_l_copy(&term, &coeffs[d]);
        /* term = coeffs[d] * x^d  (scalar multiply, mod Q) */
        polyvec_l_mul_scalar(&term, (int32_t)(power % (int64_t)Q));
        polyvec_l_add(out, out, &term);
        power = power * x_val;
    }
}

/* ------------------------------------------------------------------ */
static int run_sign_verify(
    const char *label,
    const matrix_t *A,
    const polyvec_k_t *t_master,
    party_secret_t *parties,
    const uint8_t chaincode[CHAINCODE_BYTES],
    const uint8_t *msg, size_t msglen,
    uint32_t ctr,
    const int *active_set,
    qs_prng_t *prng
)
{
    int failures = 0;
    printf("\n=== %s ===\n", label);

    /* ---- Session ID ---- */
    uint8_t sid[SESSION_ID_BYTES];
    HSessionID(sid, msg, msglen, ctr);

    /* ---- Derive rho = H(chaincode || sid) ---- */
    uint8_t rho_buf[CHAINCODE_BYTES + SESSION_ID_BYTES];
    memcpy(rho_buf, chaincode, CHAINCODE_BYTES);
    memcpy(rho_buf + CHAINCODE_BYTES, sid, SESSION_ID_BYTES);
    uint8_t rho[32];
    H(rho, rho_buf, sizeof(rho_buf));

    /*
     * ---- Generate rerandomization polynomial f_j of degree T-1 ----
     *
     * Coefficients are ETA-bounded (small) so that:
     *   ||c · f_j(0)||_inf <= TAU * ETA = 120  << BETA_Z
     *
     * f_j(x) = a_0 + a_1·x where a_0, a_1 are small polyvec_l elements.
     * This guarantees Lagrange reconstruction:
     *   Σ_k λ_k · f_j(k) = f_j(0) = a_0
     */
    polyvec_l_t f_coeffs[T_SIGNERS];
    {
        qs_prng_t fj_prng;
        prng_init(&fj_prng, rho);
        for (int d = 0; d < T_SIGNERS; d++) {
            for (int i = 0; i < QS_L; i++)
                sample_small_poly(&f_coeffs[d].vec[i], &fj_prng);
        }
    }

    /* ---- RandPK: t'_pk = t_master + A · f_j(0) ---- */
    polyvec_l_t fj0;
    eval_rerand_poly(&fj0, f_coeffs, T_SIGNERS, 0);

    polyvec_k_t Af0, t_pk;
    matrix_vec_mul(&Af0, A, &fj0);
    polyvec_k_add(&t_pk, t_master, &Af0);

    uint8_t pk_bytes[PK_BYTES];
    pk_to_bytes(pk_bytes, &t_pk);

    /* ---- RandSK: sk_{j,k} = s_k + f_j(k) for each active signer k ---- */
    polyvec_l_t sk_session[T_SIGNERS];
    for (int idx = 0; idx < T_SIGNERS; idx++) {
        int k = active_set[idx];
        polyvec_l_copy(&sk_session[idx], &parties[k - 1].share);
        polyvec_l_t fk;
        eval_rerand_poly(&fk, f_coeffs, T_SIGNERS, k);
        polyvec_l_add(&sk_session[idx], &sk_session[idx], &fk);
    }

    /* ---- Round 1: Commit ---- */
    qs_commit_share commits[T_SIGNERS];
    for (int idx = 0; idx < T_SIGNERS; idx++) {
        int k = active_set[idx];
        uint8_t pws[T_SIGNERS][32];
        for (int j = 0; j < T_SIGNERS; j++)
            memcpy(pws[j], parties[k - 1].pairwise_seeds[active_set[j] - 1], 32);
        qs_sign_commit(&commits[idx], sid, msg, msglen, A,
                       (const uint8_t (*)[32])pws, active_set, T_SIGNERS, idx, prng);
    }

    /* ---- Round 2: Aggregate + Challenge ---- */
    polyvec_k_t w_sum;
    polyvec_k_zero(&w_sum);
    for (int idx = 0; idx < T_SIGNERS; idx++)
        polyvec_k_add(&w_sum, &w_sum, &commits[idx].w);

    polyvec_k_t w_agg_rounded;
    polyvec_k_round_nuw(&w_agg_rounded, &w_sum);

    uint8_t challenge[32];
    qs_compute_challenge(challenge, &w_agg_rounded, pk_bytes, PK_BYTES, msg, msglen);

    poly_t c_poly;
    qs_expand_challenge(&c_poly, challenge);

    /* ---- Round 3: Response ---- */
    polyvec_l_t z_shares[T_SIGNERS];
    for (int idx = 0; idx < T_SIGNERS; idx++) {
        int k = active_set[idx];
        int32_t lambda = qs_lagrange_coeff_modq(k, active_set, T_SIGNERS);
        qs_sign_response(&z_shares[idx], &commits[idx].r, &sk_session[idx],
                         &commits[idx].m_col, &c_poly, lambda);
    }

    /* ---- Combine ---- */
    polyvec_l_t z_final;
    polyvec_k_t h_final;
    polyvec_l_t m_rows_arr[T_SIGNERS];
    for (int idx = 0; idx < T_SIGNERS; idx++)
        polyvec_l_copy(&m_rows_arr[idx], &commits[idx].m_row);

    qs_sign_combine(&z_final, &h_final, z_shares, m_rows_arr, T_SIGNERS,
                    A, &t_pk, &c_poly, &w_agg_rounded);

    printf("  Challenge    : %02x%02x%02x%02x...\n",
           challenge[0], challenge[1], challenge[2], challenge[3]);

    /* ---- Test A: Valid signature ---- */
    int valid = qs_verify(challenge, &z_final, &h_final, &t_pk,
                          pk_bytes, PK_BYTES, msg, msglen, A);
    if (valid) {
        printf("  [PASS] Valid signature ACCEPTED\n");
    } else {
        printf("  [FAIL] Valid signature REJECTED\n");
        failures++;
    }

    /* ---- Test B: Tampered message ---- */
    {
        const uint8_t bad[] = "TAMPERED threshold lattice signature";
        int r = qs_verify(challenge, &z_final, &h_final, &t_pk,
                          pk_bytes, PK_BYTES, bad, sizeof(bad)-1, A);
        printf(r ? "  [FAIL] Tampered message ACCEPTED (security bug!)\n"
                 : "  [PASS] Tampered message correctly REJECTED\n");
        if (r) failures++;
    }

    /* ---- Test C: Tampered z ---- */
    {
        polyvec_l_t bad_z;
        polyvec_l_copy(&bad_z, &z_final);
        bad_z.vec[0].coeffs[0] ^= 1;
        int r = qs_verify(challenge, &bad_z, &h_final, &t_pk,
                          pk_bytes, PK_BYTES, msg, msglen, A);
        printf(r ? "  [FAIL] Tampered z ACCEPTED (security bug!)\n"
                 : "  [PASS] Tampered z correctly REJECTED\n");
        if (r) failures++;
    }

    return failures;
}

/* ================================================================== */
int main(void)
{
    printf("QS-STDW Threshold Signing and Verification Test\n");
    printf("================================================\n");
    printf("Parameters: N=%d, Q=%d, K=%d, L=%d, T=%d of %d\n",
           QS_N, Q, QS_K, QS_L, T_SIGNERS, N_SIGNERS);

    int total_failures = 0;

    qs_prng_t prng;
    uint8_t root_seed[32];
    memset(root_seed, 0xAB, 32);
    prng_init(&prng, root_seed);

    /* KGen */
    printf("\n[KGen] Generating master keys...\n");
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
    printf("[KGen] mpk = (A, t): done\n");

    party_secret_t parties[N_SIGNERS];
    shamir_share_t shares[N_SIGNERS];
    qs_prng_t sp;
    uint8_t ss[32]; memset(ss, 0x55, 32); prng_init(&sp, ss);
    shamir_split(&master_s, shares, N_SIGNERS, T_SIGNERS, &sp);
    for (int i = 0; i < N_SIGNERS; i++) {
        parties[i].id = (uint32_t)(i + 1);
        parties[i].share = shares[i].value;
        for (int k = 0; k < N_SIGNERS; k++) {
            /* Symmetric pairwise seeds: seed_{i,k} = seed_{k,i} = H(min,max) */
            uint8_t ps[32];
            int lo = (i < k) ? i : k, hi = (i < k) ? k : i;
            memset(ps, (uint8_t)(lo * 7 + hi * 13 + 1), 32);
            memcpy(parties[i].pairwise_seeds[k], ps, 32);
        }
    }
    uint8_t chaincode[CHAINCODE_BYTES];
    memset(chaincode, 0xCC, CHAINCODE_BYTES);
    printf("[KGen] Shamir split among %d parties (threshold %d): done\n",
           N_SIGNERS, T_SIGNERS);

    /* Test 1: parties {1,2} */
    const uint8_t msg1[] = "threshold lattice signature test";
    int active1[] = {1, 2};
    total_failures += run_sign_verify("Test 1: Signers {1,2}", &A, &t_master,
                                      parties, chaincode, msg1, sizeof(msg1)-1,
                                      0, active1, &prng);

    /* Test 2: parties {2,3} */
    const uint8_t msg2[] = "another lattice wallet signing test";
    int active2[] = {2, 3};
    total_failures += run_sign_verify("Test 2: Signers {2,3}", &A, &t_master,
                                      parties, chaincode, msg2, sizeof(msg2)-1,
                                      1, active2, &prng);

    printf("\n================================================\n");
    if (total_failures == 0)
        printf("ALL TESTS PASSED (%d checks across 2 test cases)\n", 3 * 2);
    else
        printf("FAILED: %d test(s) did not pass\n", total_failures);

    return (total_failures == 0) ? 0 : 1;
}