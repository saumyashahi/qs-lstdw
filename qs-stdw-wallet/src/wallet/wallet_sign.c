#include "wallet.h"
#include "../common/hash.h"
#include "../threshold/rerandomize.h"
#include "../derivation/rand_pk.h"
#include "../lattice/sample.h"
#include "../signing/challenge_poly.h"
#include <string.h>

/*
 * Helper: Identify our 0-based index in the active_set.
 * Returns -1 if we are not in the active_set.
 */
static int find_active_index(uint32_t my_id, const int *active_set, int active_count)
{
    for (int i = 0; i < active_count; i++) {
        if ((uint32_t)active_set[i] == my_id) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Algorithm 4: QS-STDW.Sign(msg, {sk_{j, k}}_{k \in act}, act, pk_j)
 * Threshold Signing Protocol via Network Interaction
 * 
 * Round 1 (Commitment):
 * 1. Each party k \in act:
 * 2.   y_k <- D_gamma1^l, r_k <- D_gamma1^l
 * 3.   w_k := A * y_k + r_k mod q
 * 4.   Broadcast w_k
 * 
 * Round 2 (Challenge):
 * 5.   w := \sum_{k \in act} w_k mod q
 * 6.   c <- H(msg || w)
 * 7.   c(x) <- Expand(c)
 * 
 * Round 3 (Response):
 * 8. Each party k \in act:
 * 9.   z_k := y_k + c * s_{j, k} * \lambda_{k, act}
 * 10.  Broadcast z_k
 * 
 * Round 4 (Combine):
 * 11.  z := \sum_{k \in act} z_k
 * 12.  h <- MakeHint(w, A*z - c*t_{pk_{sess}} mod q)
 * 13.  return \sigma = (c, z, h)
 */
int wallet_sign_round1(qs_wallet_t *w,
                       const uint8_t *msg,
                       size_t msglen,
                       uint32_t ctr,
                       const int *active_set,
                       int active_count,
                       msg_commit_t *out_commit)
{
    if (!w || !msg || !active_set || !out_commit) return -1;
    if (active_count < w->threshold) return -2; /* Not enough signers */

    w->active_idx = find_active_index(w->id, active_set, active_count);
    if (w->active_idx < 0) return -3; /* We are not an active signer */

    /* Save session state */
    memcpy(w->active_msg, msg, msglen < sizeof(w->active_msg) ? msglen : sizeof(w->active_msg));
    w->active_msg_len = msglen;
    w->active_count = active_count;
    for (int i = 0; i < active_count; i++) {
        w->active_set[i] = active_set[i];
    }

    /* 1. Session ID: sid = H(msg || ctr) */
    HSessionID(w->active_session_id, msg, msglen, ctr);

    /* 
     * 2. Rerandomize Secret Share: sk_{j,k} = s_k + f_j(k) 
     * We use the threshold/rerandomize.c API. However, recall our recent polynomial 
     * fix required f_j to be a consistent polynomial. We'll use the fixed unified 
     * approach here:
     */
    
    /* rho = H(chaincode || sid) */
    uint8_t rho_buf[CHAINCODE_BYTES + SESSION_ID_BYTES];
    memcpy(rho_buf, w->chaincode, CHAINCODE_BYTES);
    memcpy(rho_buf + CHAINCODE_BYTES, w->active_session_id, SESSION_ID_BYTES);
    uint8_t rho[32];
    H(rho, rho_buf, sizeof(rho_buf));

    /* Generate f_j polynomial with small (ETA-bounded) coefficients */
    polyvec_l_t f_coeffs[MAX_PARTIES];
    qs_prng_t fj_prng;
    prng_init(&fj_prng, rho);
    for (int d = 0; d < w->threshold; d++) {
        for (int i = 0; i < QS_L; i++) {
            sample_small_poly(&f_coeffs[d].vec[i], &fj_prng);
        }
    }

    /* Eval f_j at x=k */
    polyvec_l_t fk;
    polyvec_l_zero(&fk);
    int64_t power = 1;
    for (int d = 0; d < w->threshold; d++) {
        polyvec_l_t term;
        polyvec_l_copy(&term, &f_coeffs[d]);
        polyvec_l_mul_scalar(&term, (int32_t)(power % (int64_t)Q));
        polyvec_l_add(&fk, &fk, &term);
        power = power * w->id;
    }
    
    /* Session SK share */
    polyvec_l_add(&w->session_sk_share, &w->master_share, &fk);

    /* Eval f_j at x=0 for t'_pk */
    polyvec_l_t fj0;
    polyvec_l_zero(&fj0);
    polyvec_l_copy(&fj0, &f_coeffs[0]); /* x^0 term */
    
    polyvec_k_t Af0;
    matrix_vec_mul(&Af0, w->A, &fj0);
    polyvec_k_add(&w->session_pk, w->t_master, &Af0);
    polyvec_k_to_bytes(w->session_pk_bytes, sizeof(w->session_pk_bytes), &w->session_pk);

    /* 3. Round 1 Commit Phase */
    /* 
     * Extract the relevant subset of pairwise seeds for JUST the active signers.
     * The commit.c PRF blinder loop expects an array of size `active_count` x 32,
     * where index `j` corresponds to `active_set[j]`.
     * Since `w->pairwise_seeds[k]` contains the seed shared with party `k+1`,
     * the seed with `active_set[j]` is at `w->pairwise_seeds[active_set[j] - 1]`.
     */
    uint8_t active_pws[MAX_PARTIES][32];
    for (int j = 0; j < active_count; j++) {
        int peer_id = active_set[j];
        memcpy(active_pws[j], w->pairwise_seeds[peer_id - 1], 32);
    }

    qs_sign_commit(&w->current_commit,
                   w->active_session_id,
                   w->active_msg,
                   w->active_msg_len,
                   w->A,
                   (const uint8_t (*)[32])active_pws,
                   active_set,
                   active_count,
                   w->active_idx,
                   &w->prng);

    /* Construct outgoing message */
    memset(out_commit, 0, sizeof(msg_commit_t));
    out_commit->sender_id = w->id;
    out_commit->commit_share = w->current_commit;

    return 0;
}

/*
 * Process Round 2 Challenge from the broker.
 * Computes and returns the Round 3 response share (z).
 */
int wallet_sign_round3(qs_wallet_t *w,
                       const msg_challenge_t *in_challenge,
                       msg_response_t *out_response)
{
    if (!w || !in_challenge || !out_response) return -1;
    if (w->active_idx < 0) return -2; /* Not in active session */

    /* Expand the received 32-byte challenge into a polynomial */
    poly_t c_poly;
    qs_expand_challenge(&c_poly, in_challenge->challenge);

    /* Compute our Lagrange coefficient for the active set */
    int32_t lambda = qs_lagrange_coeff_modq(w->id, w->active_set, w->active_count);

    /* Round 3: signature response */
    polyvec_l_t z_share;
    qs_sign_response(&z_share,
                     &w->current_commit.r,
                     &w->session_sk_share,
                     &w->current_commit.m_col,
                     &c_poly,
                     lambda);

    /* Construct outgoing message */
    memset(out_response, 0, sizeof(msg_response_t));
    out_response->sender_id = w->id;
    polyvec_l_copy(&out_response->z_share, &z_share);

    return 0;
}
