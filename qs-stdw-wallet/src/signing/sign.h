#ifndef QS_SIGN_H
#define QS_SIGN_H

#include <stdint.h>
#include <stddef.h>

#include "../lattice/matrix.h"
#include "../common/prng.h"
#include "../config/params.h"

/*
 * Per-party commit share (Algorithm 4, Round 1)
 * Each active signer k produces one of these.
 */
typedef struct {

    polyvec_l_t r;        /* ephemeral secret vector  r_{j,k} in [-(GAMMA1-1), GAMMA1] */
    polyvec_k_t e_prime;  /* masking noise             e'_{j,k} small */
    polyvec_k_t w;        /* commitment share          w_{j,k} = A*r + e' */

    polyvec_l_t m_row;    /* public row blinder        m_{j,k} (Alg4 line 9) */
    polyvec_l_t m_col;    /* private column blinder    m*_{j,k} (Alg4 line 18) */

    uint8_t commitment[32]; /* cmt_{j,k} = Hcom(sid, act, m, w_{j,k}) */

} qs_commit_share;


/* ---------- Commit (Round 1) ---------- */
/*
 * Algorithm 4, lines 6-10:
 *   r_{j,k}    ← D^l_w
 *   e'_{j,k}   ← D^k_w
 *   w_{j,k}    = A·r_{j,k} + e'_{j,k}
 *   cmt_{j,k}  = Hcom(sid, act, m, w_{j,k})
 *   m_{j,k}    = Σ_{i∈act} PRF(seed_{k,i}, sid)  [simplified: stored here]
 */
void qs_sign_commit(
    qs_commit_share *out,
    const uint8_t sid[32],
    const uint8_t *msg,
    size_t msglen,
    const matrix_t *A,
    const uint8_t pairwise_seeds[][32], /* seed_{k,*}: seeds of signer k with others */
    const int *active_set,
    int t,
    int signer_idx,                     /* 0-based index of this signer */
    qs_prng_t *prng
);


/* ---------- Challenge (Round 2, line 16) ---------- */
/*
 * c_j = Hc(pk_j, m_j, w_j)   where w_j = round_{ν_w}(Σ w_{j,k})
 *
 * Returns 32-byte challenge hash (expand to polynomial via qs_expand_challenge).
 */
void qs_compute_challenge(
    uint8_t c[32],
    const polyvec_k_t *w_agg_rounded,
    const uint8_t *pk_bytes,
    size_t pk_bytes_len,
    const uint8_t *msg,
    size_t msglen
);


/* ---------- Response (Round 3, line 20) ---------- */
/*
 * Algorithm 4, line 20:
 *   z_{j,k} = c · λ_{act,k} · s_{j,k} + r_{j,k} + m*_{j,k}
 *
 * where c_poly is the challenge polynomial, lambda is the Lagrange coefficient
 * for this signer within the active set (computed mod Q).
 */
void qs_sign_response(
    polyvec_l_t *z,
    const polyvec_l_t *r,
    const polyvec_l_t *secret_share,
    const polyvec_l_t *m_col_blinder,
    const poly_t *c_poly,
    int32_t lambda_modq              /* Lagrange coefficient mod Q */
);


/* ---------- Combine (lines 22-25) ---------- */
/*
 * Algorithm 4, lines 22-25:
 *   z_j      = Σ_{i∈act}(z_{j,i} - m_{j,i})   -- remove row blinders
 *   y_j      = round_{ν_w}(A·z_j - 2^{ν_t}·c·t'_pk_j)
 *   h_j      = w_j - y_j
 *   σ_j      = (c_j, z_j, h_j)
 */
void qs_sign_combine(
    polyvec_l_t *z_out,          /* z_j */
    polyvec_k_t *h_out,          /* h_j */
    const polyvec_l_t *z_shares, /* z_{j,i} for each signer */
    const polyvec_l_t *m_rows,   /* m_{j,i} row blinders */
    int t,                       /* number of active signers */
    const matrix_t *A,
    const polyvec_k_t *t_pk,     /* t'_{pk_j} rerandomized public key trapdoor */
    const poly_t *c_poly,        /* challenge polynomial */
    const polyvec_k_t *w_agg_rounded /* w_j from Round 2 aggregate */
);


/* ---------- Lagrange coefficient mod Q ---------- */
/*
 * Compute λ_{act,k} = Π_{j∈act, j≠k} (j / (j-k))  mod Q
 * using modular inverse via extended Euclidean algorithm.
 *
 * signer_id  : 1-based ID of this signer
 * active_set : 1-based IDs of active signers
 * t          : |active_set| = threshold
 */
int32_t qs_lagrange_coeff_modq(
    int signer_id,
    const int *active_set,
    int t
);


/* ---------- Rounding ---------- */
/*
 * Applies round_{ν_w}: out[i] = floor(in[i] / 2^NU_W)
 * (keeps coefficients as centered integers — used for w_agg)
 */
void qs_round_nuw(
    polyvec_k_t *out,
    const polyvec_k_t *in
);


/* ---------- Serialise polyvec_k to bytes (for Hc pk argument) ---------- */
void polyvec_k_to_bytes(uint8_t *out, size_t outlen, const polyvec_k_t *pk);

#endif /* QS_SIGN_H */