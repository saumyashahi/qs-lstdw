#include <string.h>
#include "verify.h"
#include "../signing/sign.h"
#include "../signing/challenge_poly.h"
#include "../common/hash.h"
#include "../lattice/matrix.h"
#include "../lattice/polyvec.h"
#include "../../config/params.h"

int qs_verify(
    const uint8_t challenge[32],
    const polyvec_l_t *z,
    const polyvec_k_t *h,
    const polyvec_k_t *t_pk,
    const uint8_t *pk_bytes,
    size_t pk_bytes_len,
    const uint8_t *msg,
    size_t msglen,
    const matrix_t *A
)
{
    /* Recompute w' = UseHint(h, Az - c*t_pk) */
    polyvec_k_t Az;
    matrix_vec_mul(&Az, A, z);

    poly_t c_poly;
    qs_expand_challenge(&c_poly, challenge);

    polyvec_k_t ct;
    polyvec_k_mul_poly(&ct, &c_poly, t_pk);

    /* Scale c*t by 2^nu_t before subtracting (match signing convention) */
    polyvec_k_t scaled;
    polyvec_k_shift_left(&scaled, &ct, NU_T);

    polyvec_k_t v;
    polyvec_k_sub(&v, &Az, &scaled);

    /* Round v down by 2^nu_w */
    polyvec_k_t y;
    polyvec_k_round_nuw(&y, &v);

    /* w' = h + y */
    polyvec_k_t w_prime;
    polyvec_k_add(&w_prime, h, &y);

    /* Recompute challenge and compare */
    uint8_t challenge_check[32];
    qs_compute_challenge(challenge_check, &w_prime,
                         pk_bytes, pk_bytes_len, msg, msglen);

    if (memcmp(challenge, challenge_check, 32) != 0)
        return 0;

    /* Norm checks */
    if (polyvec_l_norm_inf(z)       >= BETA_Z) return 0;
    if (polyvec_k_norm_inf(&w_prime) >= BETA_W) return 0;
    if (polyvec_k_norm_inf(h)        >= BETA_W) return 0;

    return 1;
}
