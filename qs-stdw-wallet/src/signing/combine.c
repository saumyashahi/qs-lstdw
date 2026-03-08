#include "sign.h"
#include "../lattice/polyvec.h"
#include "../lattice/matrix.h"
#include "../config/params.h"

/*
 * Combine Phase — Algorithm 4, lines 22-25
 *
 * Line 22: z_j = Σ_{i∈act}(z_{j,i} - m_{j,i})       -- remove row blinders
 * Line 23: y_j = round_{ν_w}(A·z_j - 2^{ν_t}·c_j·t'_{pk_j})
 * Line 24: h_j = w_j - y_j                            -- hint
 * Line 25: σ_j = (c_j, z_j, h_j)
 *
 * Note on blinder cancellation:
 *   Σ_{k∈act} m*_{j,k} = Σ_{k∈act} m_{j,k}  by pairwise symmetry of PRF seeds,
 *   so the blinders cancel in the combined z_j = c·Σ_k λ_k s_{j,k} + Σ_k r_{j,k}
 *                                              = c · s_j + r_j
 *   where s_j is the reconstructed master secret (from Shamir interpolation).
 */

void qs_sign_combine(
    polyvec_l_t *z_out,
    polyvec_k_t *h_out,
    const polyvec_l_t *z_shares,     /* z_{j,i} for each of t signers */
    const polyvec_l_t *m_rows,       /* m_{j,i} row blinders for each of t signers */
    int t,
    const matrix_t *A,
    const polyvec_k_t *t_pk,         /* t'_{pk_j}: rerandomized public key trapdoor */
    const poly_t *c_poly,            /* challenge polynomial c_j */
    const polyvec_k_t *w_agg_rounded /* w_j = round_{ν_w}(Σ w_{j,i}) */
)
{
    /* --- Line 22: z_j = Σ_{i∈act}(z_{j,i} - m_{j,i}) --- */
    polyvec_l_zero(z_out);
    for (int i = 0; i < t; i++) {
        polyvec_l_t diff;
        polyvec_l_sub(&diff, &z_shares[i], &m_rows[i]);
        polyvec_l_add(z_out, z_out, &diff);
    }

    /* --- Line 23: y_j = round_{ν_w}(A·z_j - 2^{ν_t}·c_j·t'_{pk_j}) --- */

    /* Step 23a: Az = A * z_j */
    polyvec_k_t Az;
    matrix_vec_mul(&Az, A, z_out);

    /* Step 23b: ct = c_j * t'_{pk_j}  (challenge polynomial × public key trapdoor) */
    polyvec_k_t ct;
    polyvec_k_mul_poly(&ct, c_poly, t_pk);

    /* Step 23c: scaled = 2^{ν_t} * ct */
    polyvec_k_t scaled;
    polyvec_k_shift_left(&scaled, &ct, NU_T);

    /* Step 23d: v = Az - scaled */
    polyvec_k_t v;
    polyvec_k_sub(&v, &Az, &scaled);

    /* Step 23e: y_j = round_{ν_w}(v) */
    polyvec_k_t y;
    polyvec_k_round_nuw(&y, &v);

    /* --- Line 24: h_j = w_j - y_j --- */
    polyvec_k_sub(h_out, w_agg_rounded, &y);
}