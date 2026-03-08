#include "rand_pk.h"
#include "../common/hash.h"
#include "../common/prng.h"
#include <string.h>

/**
 * @brief Algorithm 3: QS-STDW.RandPK(mpk, seed_{i, j}, {sessID_j}_{j=1}^s)
 * Rerandomize Public Key for Multiple Sessions
 * 
 * 1. for j = 1 to s do
 * 2.   rho_j <- H(seed_{i, j} || sessID_j)
 * 3.   f_j(x) <- POLYFROMSEED(rho_j, T - 1, q)
 * 4.   t_{pk_j} <- t_{pk} + A * f_j(0) mod q
 * 5.   pk_j <- (A, t_{pk_j})
 * 6. return {pk_j}_{j=1}^s
 * 
 * Note: This implementation processes a single session_id per call and evaluates
 * the polynomial at x=0 by directly expanding the pseudo-random vector.
 */
void derive_session_pk(
    session_pk_t *out,
    const public_key_t *mpk,
    const uint8_t *seed,
    const uint8_t *session_id)
{
    uint8_t input[64];
    uint8_t rho[32];

    /* rho_j = H(seed || session_id) */
    memcpy(input, seed, 32);
    memcpy(input + 32, session_id, 32);
    H(rho, input, sizeof(input));

    /* derive rerandomization vector f (length L) */
    qs_prng_t prng;
    prng_init(&prng, rho);

    polyvec_l_t f;
    polyvec_l_uniform(&f, &prng);

    /* compute Af = A * f (length K) */
    polyvec_k_t Af;
    matrix_vec_mul(&Af, &mpk->A, &f);

    /* t' = t + Af */
    polyvec_k_add(&out->t_prime, &mpk->t, &Af);

    /* matrix unchanged */
    out->A = mpk->A;
}