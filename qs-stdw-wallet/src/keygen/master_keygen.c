#include "master_keygen.h"
#include "../common/prng.h"
#include "../common/randombytes.h"
#include "../threshold/shamir.h"
#include "../threshold/share.h"
#include "../lattice/matrix.h"
#include "../lattice/sample.h"
#include "../lattice/poly.h"
#include "../lattice/polyvec.h"
#include <string.h>

/**
 * @brief Algorithm 1: QS-STDW.KGen(lambda, T, N)
 * Threshold Lattice-Based Deterministic Wallet Master Key Generation
 * 
 * 1. A <- R_q^{k \times l}
 * 2. (msk, e) <- D_{t}^{l} \times D_{t}^{k}
 * 3. t := A * msk + e
 * 4. mpk := (A, t)
 * 5. P <- R_q^l[X] with deg(P) = T - 1, P(0) = msk
 * 6. (s_i)_{i \in [N]} := (P(i))_{i \in [N]}
 * 7. (pk_{sig, i}, sk_{sig, i}) <- KeyGen_{sig}(1^kappa) for i \in [N]
 * 8. seed_{i, k} <- {0, 1}^kappa for i, k \in [N]
 * 9. msk_i := (s_i, (pk_{sig, k})_{k \in [N]}, sk_{sig, i}, (seed_{i, k}, seed_{k, i})_{k \in [N]})
 * 10. ch <- RANDOMBITS(lambda)
 * 11. return (mpk, {msk_i}_{i=1}^N, ch)
 */
void master_keygen(master_public_key_t *mpk,
                   party_secret_t parties[MAX_PARTIES],
                   uint8_t chaincode[CHAINCODE_BYTES],
                   int N,
                   int T)
{
    uint8_t seed_A[SEED_BYTES];

    /* Deterministic PRNG for testing */
    qs_prng_t prng;
    uint8_t seed[32];
    memset(seed, 0x42, 32);
    prng_init(&prng, seed);

    /* 1. Sample seed for A */
    prng_squeeze(&prng, seed_A, SEED_BYTES);

    /* 2. Expand matrix A */
    matrix_expand(&mpk->A, seed_A);

    /* 3. Sample master secret vector s */
    polyvec_l_t master_s;
    for (int i = 0; i < QS_L; i++) {
        sample_small_poly(&master_s.vec[i], &prng);
    }

    /* 4. Sample error vector e */
    polyvec_k_t e;
    for (int i = 0; i < QS_K; i++) {
        sample_small_poly(&e.vec[i], &prng);
    }

    /* 5. Compute t = A*s + e */
    matrix_vec_mul(&mpk->t, &mpk->A, &master_s);

    for (int i = 0; i < QS_K; i++) {
        poly_add(&mpk->t.vec[i],
                 &mpk->t.vec[i],
                 &e.vec[i]);
    }

    /* 6. Store seed_A */
    memcpy(mpk->seed_A, seed_A, SEED_BYTES);

    /* 7. Shamir split master secret */
    shamir_share_t shares[MAX_PARTIES];
    shamir_split(&master_s,
                 shares,
                 N,
                 T,
                 &prng);

    /* 8. Assign shares to parties */
    for (int i = 0; i < N; i++) {
        parties[i].id = i + 1;   // Shamir points start at 1
        parties[i].share = shares[i].value;
    }

    /* 9. Generate pairwise seeds */
    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            randombytes(parties[i].pairwise_seeds[k], SEED_BYTES);
        }
    }

    /* 10. Stub signature keys (placeholder) */
    for (int i = 0; i < N; i++) {
        randombytes(parties[i].sig_sk, SIG_SK_BYTES);
        randombytes(parties[i].sig_pk, SIG_PK_BYTES);
    }

    /* 11. Generate chaincode */
    randombytes(chaincode, CHAINCODE_BYTES);

    for (int i = 0; i < N; i++) {
        memcpy(parties[i].chaincode, chaincode, CHAINCODE_BYTES);
    }
}