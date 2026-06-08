#include "sign.h"
#include "challenge_poly.h"
#include "../lattice/polyvec.h"
#include "../../config/params.h"

/*
 * Response Phase — Algorithm 4, lines 18-21
 *
 * z_{j,k} = c · λ_{act,k} · s_{j,k} + r_{j,k} + m*_{j,k}
 *
 * lambda_modq is int64_t to handle Raccoon's 49-bit q.
 */
void qs_sign_response(
    polyvec_l_t *z,
    const polyvec_l_t *r,
    const polyvec_l_t *secret_share,
    const polyvec_l_t *m_col_blinder,
    const poly_t *c_poly,
    int64_t lambda_modq
)
{
    polyvec_l_t cs;  /* c * s_{j,k} */

    /* Step 1: cs = c_poly * secret_share */
    polyvec_l_mul_poly(&cs, c_poly, secret_share);

    /* Step 2: cs = λ * cs  (scalar multiply mod q) */
    polyvec_l_mul_scalar(&cs, lambda_modq);

    /* Step 3: z = r + cs + m_col */
    polyvec_l_add(z, r, &cs);
    polyvec_l_add(z, z, m_col_blinder);
}
