#ifndef QS_SIGN_H
#define QS_SIGN_H

#include <stdint.h>
#include <stddef.h>

#include "../lattice/matrix.h"
#include "../common/prng.h"
#include "../../config/params.h"

/*
 * Per-party commit share (Algorithm 4, Round 1)
 */
typedef struct {
    polyvec_l_t r;          /* ephemeral secret vector  r_{j,k} */
    polyvec_k_t e_prime;    /* masking noise             e'_{j,k} */
    polyvec_k_t w;          /* commitment share          w_{j,k} = A*r + e' */
    polyvec_l_t m_row;      /* public row blinder        m_{j,k} */
    polyvec_l_t m_col;      /* private column blinder    m*_{j,k} */
    uint8_t commitment[32]; /* cmt_{j,k} = Hcom(sid, act, m, w_{j,k}) */
} qs_commit_share;


/* ---------- Commit (Round 1) ---------- */
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
);

/* ---------- Challenge (Round 2) ---------- */
void qs_compute_challenge(
    uint8_t c[32],
    const polyvec_k_t *w_agg_rounded,
    const uint8_t *pk_bytes,
    size_t pk_bytes_len,
    const uint8_t *msg,
    size_t msglen
);

/* ---------- Response (Round 3) ---------- */
void qs_sign_response(
    polyvec_l_t *z,
    const polyvec_l_t *r,
    const polyvec_l_t *secret_share,
    const polyvec_l_t *m_col_blinder,
    const poly_t *c_poly,
    int64_t lambda_modq            /* int64_t — Raccoon q is 49-bit */
);

/* ---------- Combine (lines 22-25) ---------- */
void qs_sign_combine(
    polyvec_l_t *z_out,
    polyvec_k_t *h_out,
    const polyvec_l_t *z_shares,
    const polyvec_l_t *m_rows,
    int t,
    const matrix_t *A,
    const polyvec_k_t *t_pk,
    const poly_t *c_poly,
    const polyvec_k_t *w_agg_rounded
);

/* ---------- Lagrange coefficient mod Q (returns int64_t) ---------- */
int64_t qs_lagrange_coeff_modq(int signer_id, const int *active_set, int t);

/* ---------- Rounding ---------- */
void qs_round_nuw(polyvec_k_t *out, const polyvec_k_t *in);

/* ---------- Serialise polyvec_k to bytes (7 bytes/coeff) ---------- */
void polyvec_k_to_bytes(uint8_t *out, size_t outlen, const polyvec_k_t *pk);

#endif /* QS_SIGN_H */
