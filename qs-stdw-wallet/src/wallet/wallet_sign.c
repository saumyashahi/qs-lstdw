#include "wallet.h"
#include "../common/hash.h"
#include "../threshold/rerandomize.h"
#include "../derivation/rand_pk.h"
#include "../lattice/sample.h"
#include "../signing/challenge_poly.h"
#include <string.h>
#include <stdlib.h>

static int find_active_index(uint32_t my_id, const int *active_set, int active_count)
{
    for (int i = 0; i < active_count; i++)
        if ((uint32_t)active_set[i] == my_id) return i;
    return -1;
}

int wallet_sign_round1(qs_wallet_t *w,
                       const uint8_t *msg,
                       size_t msglen,
                       uint32_t ctr,
                       const int *active_set,
                       int active_count,
                       msg_commit_t *out_commit)
{
    if (!w || !msg || !active_set || !out_commit) return -1;
    if (active_count < w->threshold) return -2;

    w->active_idx = find_active_index(w->id, active_set, active_count);
    if (w->active_idx < 0) return -3;

    size_t cpylen = msglen < sizeof(w->active_msg) ? msglen : sizeof(w->active_msg);
    memcpy(w->active_msg, msg, cpylen);
    w->active_msg_len = msglen;
    w->active_count   = active_count;
    for (int i = 0; i < active_count; i++)
        w->active_set[i] = active_set[i];

    /* 1. sid = H(msg || ctr) */
    HSessionID(w->active_session_id, msg, msglen, ctr);

    /* 2. Derive rerandomisation polynomial seed: rho = H(chaincode || sid) */
    uint8_t rho_buf[CHAINCODE_BYTES + SESSION_ID_BYTES];
    memcpy(rho_buf,                   w->chaincode,          CHAINCODE_BYTES);
    memcpy(rho_buf + CHAINCODE_BYTES, w->active_session_id, SESSION_ID_BYTES);
    uint8_t rho[32];
    H(rho, rho_buf, sizeof(rho_buf));

    /*
     * Generate rerandomisation polynomial f_j(x) with ternary {-1,0,+1} coefficients.
     * Raccoon uses ternary secrets — f_coeffs[d] is the coefficient of x^{d+1}.
     * (Constant term f_coeffs[0] is the x^1 coefficient; evaluated at 0 gives 0.)
     */
    int T = w->threshold;
    polyvec_l_t *f_coeffs = malloc(T * sizeof(polyvec_l_t));
    if (!f_coeffs) return -4;

    qs_prng_t fj_prng;
    prng_init(&fj_prng, rho);
    for (int d = 0; d < T; d++)
        for (int i = 0; i < QS_L; i++)
            sample_small_poly(&f_coeffs[d].vec[i], &fj_prng);

    /* Evaluate f_j(k) at x = our id (1-based) */
    polyvec_l_t fk;
    polyvec_l_zero(&fk);
    {
        __int128 power = w->id;
        for (int d = 0; d < T; d++) {
            polyvec_l_t term;
            polyvec_l_copy(&term, &f_coeffs[d]);
            int64_t pw = (int64_t)(power % RACCOON_Q);
            polyvec_l_mul_scalar(&term, pw);
            polyvec_l_add(&fk, &fk, &term);
            power = (power * w->id) % RACCOON_Q;
        }
    }

    /* Session SK share: sk_{j,k} = master_share + f_j(k) */
    polyvec_l_add(&w->session_sk_share, &w->master_share, &fk);

    /* Evaluate f_j(0): only x^0 constant term — but we defined f_j(x)=a1*x+...
     * so f_j(0) = 0.  For PK rerandomisation we use rand_pk.c / derive_session_pk
     * which already derives the public key from the chain code + session ID. */
    {
        polyvec_l_t fj0;
        polyvec_l_zero(&fj0);
        /* f_j evaluated at 0: all terms have x^{>=1}, so result = 0 */
        polyvec_k_t Af0;
        matrix_vec_mul(&Af0, w->A, &fj0);
        polyvec_k_add(&w->session_pk, w->t_master, &Af0);
    }
    polyvec_k_to_bytes(w->session_pk_bytes, sizeof(w->session_pk_bytes), &w->session_pk);

    /* 3. Build pairwise seed array for active signers */
    uint8_t (*active_pws)[32] = malloc(active_count * 32);
    if (!active_pws) { free(f_coeffs); return -4; }
    for (int j = 0; j < active_count; j++) {
        int peer_id = active_set[j];
        memcpy(active_pws[j], w->pairwise_seeds[peer_id - 1], 32);
    }

    /* 4. Round 1 commit */
    qs_sign_commit(&w->current_commit,
                   w->active_session_id,
                   w->active_msg,
                   w->active_msg_len,
                   w->A,
                   (const uint8_t (*)[32])active_pws,
                   active_set, active_count,
                   w->active_idx, &w->prng);

    memset(out_commit, 0, sizeof(msg_commit_t));
    out_commit->sender_id    = w->id;
    out_commit->commit_share = w->current_commit;

    free(f_coeffs);
    free(active_pws);
    return 0;
}

int wallet_sign_round3(qs_wallet_t *w,
                       const msg_challenge_t *in_challenge,
                       msg_response_t *out_response)
{
    if (!w || !in_challenge || !out_response) return -1;
    if (w->active_idx < 0) return -2;

    poly_t c_poly;
    qs_expand_challenge(&c_poly, in_challenge->challenge);

    /* int64_t Lagrange coefficient — Raccoon q is 49-bit */
    int64_t lambda = qs_lagrange_coeff_modq(w->id, w->active_set, w->active_count);

    polyvec_l_t z_share;
    qs_sign_response(&z_share,
                     &w->current_commit.r,
                     &w->session_sk_share,
                     &w->current_commit.m_col,
                     &c_poly,
                     lambda);

    memset(out_response, 0, sizeof(msg_response_t));
    out_response->sender_id = w->id;
    polyvec_l_copy(&out_response->z_share, &z_share);
    return 0;
}
