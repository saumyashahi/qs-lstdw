#include "rerandomize.h"
#include "../common/hash.h"
#include "../common/prng.h"
#include "../lattice/matrix.h"
#include "../lattice/poly.h"
#include "../lattice/polyvec.h"
#include "../../config/params.h"
#include <string.h>
#include <stdlib.h>

/* Derive 32-byte seed = H(chaincode || session_id) */
static void derive_seed(uint8_t out[32],
                        const uint8_t chaincode[CHAINCODE_BYTES],
                        const uint8_t *session_id,
                        size_t session_len)
{
    uint8_t buf[CHAINCODE_BYTES + 64];
    memcpy(buf, chaincode, CHAINCODE_BYTES);
    memcpy(buf + CHAINCODE_BYTES, session_id, session_len);
    shake256(out, 32, buf, CHAINCODE_BYTES + session_len);
}

/* Generate T-1 random polyvec_l coefficients for the rerandomization poly */
static void generate_polyvec_coeffs(polyvec_l_t *coeffs, int T, const uint8_t seed[32])
{
    qs_prng_t prng;
    prng_init(&prng, seed);
    for (int i = 0; i < T - 1; i++)
        polyvec_l_uniform(&coeffs[i], &prng);
}

/* Evaluate f(index) = a1*index + a2*index^2 + ... mod q */
static void eval_polyvec(polyvec_l_t *result,
                         polyvec_l_t *coeffs,
                         int T,
                         int index)
{
    polyvec_l_zero(result);
    polyvec_l_t temp;
    int64_t power = index;

    for (int k = 0; k < T - 1; k++) {
        polyvec_l_copy(&temp, &coeffs[k]);
        polyvec_l_mul_scalar(&temp, power);
        polyvec_l_add(result, result, &temp);
        power = ((__int128)power * index) % RACCOON_Q;
    }
}

/*
 * Algorithm 2: QS-STDW.RandSK — rerandomize secret shares for one session.
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

    if (T <= 1) return;   /* nothing to rerandomize for T=1 */

    polyvec_l_t *coeffs = malloc((T - 1) * sizeof(polyvec_l_t));
    if (!coeffs) return;

    generate_polyvec_coeffs(coeffs, T, seed);

    for (int i = 0; i < N; i++) {
        polyvec_l_t delta;
        eval_polyvec(&delta, coeffs, T, i + 1);   /* 1-based index */
        polyvec_l_add(&parties[i].share, &parties[i].share, &delta);
    }

    free(coeffs);
}
