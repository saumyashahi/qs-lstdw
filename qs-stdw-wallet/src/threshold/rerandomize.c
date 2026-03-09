#include "rerandomize.h"
#include "../common/hash.h"
#include "../common/prng.h"
#include "../lattice/matrix.h"
#include "../lattice/poly.h"
#include "../lattice/polyvec.h"
#include <string.h>
#include <stdlib.h>

/* derive 32-byte seed = H(chaincode || session_id) */
static void derive_seed(uint8_t out[32],
                        const uint8_t chaincode[CHAINCODE_BYTES],
                        const uint8_t *session_id,
                        size_t session_len)
{
    uint8_t buffer[CHAINCODE_BYTES + 64];

    memcpy(buffer, chaincode, CHAINCODE_BYTES);
    memcpy(buffer + CHAINCODE_BYTES, session_id, session_len);

    shake256(out, 32, buffer, CHAINCODE_BYTES + session_len);
}

/* generate T-1 polyvec_l coefficients (no constant term) */

static void generate_polyvec_coeffs(polyvec_l_t *coeffs,
                                    int T,
                                    const uint8_t seed[32])
{
    qs_prng_t prng;
    prng_init(&prng, seed);

    for (int i = 0; i < T - 1; i++) {
        polyvec_l_uniform(&coeffs[i], &prng);
    }
}

/* evaluate f(i) = a1*i + a2*i^2 + ...  */
static void eval_polyvec(polyvec_l_t *result,
                         polyvec_l_t *coeffs,
                         int T,
                         int index)
{
    polyvec_l_zero(result);

    polyvec_l_t temp;
    int power = index;

    for (int k = 0; k < T - 1; k++) {

        polyvec_l_copy(&temp, &coeffs[k]);

        /* multiply each coefficient by scalar power */
        polyvec_l_mul_scalar(&temp, power);

        polyvec_l_add(result, result, &temp);

        power *= index;
    }
}

/**
 * @brief Algorithm 2: QS-STDW.RandSK({msk_i}_{i=1}^N, ch, {sessID_j}_{j=1}^s)
 * Rerandomize Secret Shares for Multiple Sessions
 * 
 * 1. for j = 1 to s do:
 * 2.   rho_j <- H(ch || sessID_j)
 * 3.   f_j(x) <- POLYFROMSEED(rho_j, T - 1, q)
 * 4.   for i = 1 to N do:
 * 5.     parse msk_i as (s_i, ...)
 * 6.     s_{j, i} <- s_i + f_j(i) mod q
 * 7.     sk_{j, i} <- (s_{j, i}, ...)
 * 8. return {sk_{j, i}}_{j=1..s, i=1..N}
 * 
 * Note: This implementation processes a single session_id per call.
 */
void rerandomize_shares(party_secret_t parties[],
                        int N,
                        int T,
                        const uint8_t chaincode[CHAINCODE_BYTES],
                        const uint8_t *session_id,
                        size_t session_len)
{
    uint8_t seed[32];
    derive_seed(seed, chaincode, session_id, session_len);

    polyvec_l_t *coeffs = malloc((T - 1) * sizeof(polyvec_l_t));
    if (!coeffs) return; // Allocation failed

    generate_polyvec_coeffs(coeffs, T, seed);

    for (int i = 0; i < N; i++) {

        polyvec_l_t delta;

        /* assuming party indices are 1-based */
        int index = i + 1;

        eval_polyvec(&delta, coeffs, T, index);

        polyvec_l_add(&parties[i].share,
                      &parties[i].share,
                      &delta);
    }

    free(coeffs);
}