#include <stdio.h>
#include <string.h>

#include "../src/derivation/rand_pk.h"
#include "../src/common/prng.h"
#include "../src/lattice/matrix.h"
#include "../src/lattice/polyvec.h"

int main()
{
    public_key_t mpk;
    session_pk_t pk1, pk2;

    uint8_t seedA[32] = {1};
    uint8_t seed[32]  = {2};
    uint8_t sid1[32]  = {3};
    uint8_t sid2[32]  = {4};

    qs_prng_t prng;
    prng_init(&prng, seed);

    /* expand deterministic matrix */
    matrix_expand(&mpk.A, seedA);

    /* random test public key vector */
    polyvec_k_uniform(&mpk.t, &prng);

    derive_session_pk(&pk1, &mpk, seed, sid1);
    derive_session_pk(&pk2, &mpk, seed, sid1);

    if (polyvec_k_equal(&pk1.t_prime, &pk2.t_prime))
        printf("Deterministic derivation OK\n");
    else
        printf("ERROR: deterministic derivation failed\n");

    derive_session_pk(&pk2, &mpk, seed, sid2);

    if (!polyvec_k_equal(&pk1.t_prime, &pk2.t_prime))
        printf("Session IDs produce different keys\n");
    else
        printf("ERROR: session keys identical\n");

    return 0;
}