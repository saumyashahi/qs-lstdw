#include "sign.h"
#include "challenge_poly.h"
#include "../lattice/polyvec.h"
#include "../config/params.h"
#include <string.h>

/*
 * Response Phase — Algorithm 4, lines 18-21
 *
 * z_{j,k} = c · λ_{act,k} · s_{j,k} + r_{j,k} + m*_{j,k}
 *
 * where:
 *   c_poly        = challenge polynomial (ternary, from qs_expand_challenge)
 *   lambda_modq   = Lagrange coefficient λ_{act,k} ∈ [0, Q-1]
 *   secret_share  = s_{j,k}  (rerandomized session secret share of party k)
 *   r             = r_{j,k}  (ephemeral vector from commit phase)
 *   m_col_blinder = m*_{j,k} (private column blinder from commit phase)
 *
 * Computation:
 *   1. tmp = poly_mul(c_poly, s_{j,k})             — ring multiplication per component
 *   2. tmp = λ · tmp  (scalar multiply by lambda_modq)
 *   3. z   = r + tmp + m*_{j,k}
 */
void qs_sign_response(
    polyvec_l_t *z,
    const polyvec_l_t *r,
    const polyvec_l_t *secret_share,
    const polyvec_l_t *m_col_blinder,
    const poly_t *c_poly,
    int32_t lambda_modq
)
{
    polyvec_l_t cs; /* c * s_{j,k} */

    /* Step 1: cs = c_poly * secret_share  (component-wise ring multiply) */
    polyvec_l_mul_poly(&cs, c_poly, secret_share);

    /* Step 2: cs = λ * cs  (scalar multiply mod Q) */
    polyvec_l_mul_scalar(&cs, lambda_modq);

    /* Step 3: z = r + cs + m_col */
    polyvec_l_add(z, r, &cs);
    polyvec_l_add(z, z, m_col_blinder);
}