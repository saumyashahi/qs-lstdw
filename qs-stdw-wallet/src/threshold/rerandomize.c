#include "rerandomize.h"
#include "../common/hash.h"
#include "../common/prng.h"
#include "../lattice/matrix.h"
#include "../lattice/poly.h"
#include "../lattice/polyvec.h"
#include <string.h>


//for error 1 : which was missing definition of hash_sha256, ig doing #include<openssl/sha.h> and define hash_sha256 as a wrapper around SHA256 function from OpenSSL library which has the implementation readt in it (does 32 bytes)

//for error 2 : which was missing definition of polyvec_l_uniform, ig define it as a function that fills a polyvec_l_t with random polynomials using the provided PRNG
/*
qs_prng_t prng;
prng_init(&prng, seed);
*/

//for error 3 : which was missing definition of polyvec_l_zero, ig define it as a function that sets all coefficients of a polyvec_l_t to zero
/*
polyvec_l_zero(result); bakwas
*/

//for error 4 : which was missing definition of polyvec_l_copy, ig define it as a function that copies one polyvec_l_t to another
/*loop to copy type shi*/

//for error 5 : which was missing definition of polyvec_l_mul_scalar, ig define it as a function that multiplies each coefficient of a polyvec_l_t by a scalar value

/*
error 5,6 are very run a for loop type shi to do shi
polyvec_l_mul_scalar(&temp, power);
polyvec_l_add(result, result, &temp);

*/
//for error 6 : which was missing definition of polyvec_l_add, ig define it as a function that adds two polyvec_l_t together and stores the result in a third polyvec_l_t   


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

void rerandomize_shares(party_secret_t parties[],
                        int N,
                        int T,
                        const uint8_t chaincode[CHAINCODE_BYTES],
                        const uint8_t *session_id,
                        size_t session_len)
{
    uint8_t seed[32];
    derive_seed(seed, chaincode, session_id, session_len);

    polyvec_l_t coeffs[T - 1];
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
}