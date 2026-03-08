/*
 * local_sim.c
 * 
 * An in-memory network broker simulating the QS-STDW Threshold Signing protocol.
 * 
 * Flow:
 * 1. Broker spins up N offline wallets.
 * 2. User requests a signature from T active wallets.
 * 3. Broker tells active wallets to start: MSG_SIGN_INIT (Round 1).
 *    -> Wallets respond with MSG_ROUND1_COMMIT.
 * 4. Broker aggregates commitments, forms challenge: MSG_ROUND2_CHALL.
 *    -> Wallets respond with MSG_ROUND3_RESP.
 * 5. Broker combines responses into final MSG_SIGN_DONE signature.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../config/params.h"
#include "../common/prng.h"
#include "../common/hash.h"
#include "../lattice/matrix.h"
#include "../lattice/sample.h"
#include "../threshold/shamir.h"
#include "../signing/challenge_poly.h"

#include "../wallet/wallet.h"
#include "message.h"

#define N 5
#define T 3

qs_wallet_t wallets[N];

int main(void)
{
    printf("=========================================================\n");
    printf("   QS-STDW THRESHOLD SIGNING (NETWORK SIMULATION)        \n");
    printf("=========================================================\n");
    printf("Spinning up %d wallet nodes (Threshold = %d)...\n", N, T);

    /* ---------------------------------------------------------
     * 1. KGen Phase (Trusted Setup for Simulation)
     *--------------------------------------------------------- */
    qs_prng_t env_prng;
    uint8_t seed[32]; memset(seed, 0x11, 32);
    prng_init(&env_prng, seed);

    uint8_t seed_A[32]; prng_squeeze(&env_prng, seed_A, 32);
    matrix_t A; matrix_expand(&A, seed_A);

    polyvec_l_t master_s;
    for (int i=0; i<QS_L; i++) sample_small_poly(&master_s.vec[i], &env_prng);
    
    polyvec_k_t e;
    for (int i=0; i<QS_K; i++) sample_small_poly(&e.vec[i], &env_prng);
    
    polyvec_k_t t_master;
    matrix_vec_mul(&t_master, &A, &master_s);
    polyvec_k_add(&t_master, &t_master, &e);

    shamir_share_t shares[N];
    qs_prng_t sp_prng; uint8_t ss[32]; memset(ss, 0x22, 32); prng_init(&sp_prng, ss);
    shamir_split(&master_s, shares, N, T, &sp_prng);

    uint8_t chaincode[CHAINCODE_BYTES]; memset(chaincode, 0xCC, CHAINCODE_BYTES);

    uint8_t pairwise[N][N][32];
    for (int i=0; i<N; i++) {
        for (int k=0; k<N; k++) {
            int lo = (i<k) ? i : k;
            int hi = (i<k) ? k : i;
            memset(pairwise[i][k], (uint8_t)(lo*7 + hi*13 + 1), 32);
        }
    }

    /* Initialize Wallets */
    for (int i=0; i<N; i++) {
        uint8_t ws[32]; prng_squeeze(&env_prng, ws, 32);
        wallet_init(&wallets[i], i+1, &shares[i].value, 
                    (const uint8_t (*)[32])pairwise[i], chaincode, 
                    &A, &t_master, N, T, ws);
    }
    printf("> Wallets Initialized & Network Ready.\n\n");

    /* ---------------------------------------------------------
     * 2. Signature Request
     *--------------------------------------------------------- */
    const uint8_t msg[] = "TRANSFER 10.0 TO ADDRESS 0xXYZ";
    size_t msglen = sizeof(msg) - 1;
    uint32_t ctr = 1;
    
    int active_set[T] = {1, 3, 4}; /* Wallets 1, 3, 4 will sign */
    printf("BROKER: Requesting signature on MSG: '%s'\n", msg);
    printf("BROKER: Active Signer Set: { ");
    for(int i=0; i<T; i++) printf("W%d ", active_set[i]);
    printf("}\n\n");

    /* ---------------------------------------------------------
     * 3. MSG_ROUND1_COMMIT (BROKER -> WALLETS -> BROKER)
     *--------------------------------------------------------- */
    printf("BROKER: Broadcasting MSG_SIGN_INIT...\n");
    msg_commit_t commits[T];
    polyvec_l_t m_rows_from_commits[T];

    for (int i = 0; i < T; i++) {
        int w_idx = active_set[i] - 1;
        printf("  -> WALLET %d: Computing Round 1 Commit...\n", wallets[w_idx].id);
        int rc = wallet_sign_round1(&wallets[w_idx], msg, msglen, ctr, 
                                    active_set, T, &commits[i]);
        if (rc != 0) {
            printf("  -> WALLET %d ERROR: code %d\n", wallets[w_idx].id, rc);
            return 1;
        }
    }

    /* ---------------------------------------------------------
     * 4. BROKER LOGIC: AGGREGATE W
     *--------------------------------------------------------- */
    polyvec_k_t w_sum;
    polyvec_k_zero(&w_sum);
    for (int i = 0; i < T; i++) {
        polyvec_k_add(&w_sum, &w_sum, &commits[i].commit_share.w);
        polyvec_l_copy(&m_rows_from_commits[i], &commits[i].commit_share.m_row);
    }

    msg_challenge_t challenge_msg;
    polyvec_k_round_nuw(&challenge_msg.w_agg_rounded, &w_sum);

    /* Broker fetches the T_PK from any wallet (they all derived the same one deterministically) */
    uint8_t pk_bytes[QS_K * QS_N * 4];
    polyvec_k_to_bytes(pk_bytes, sizeof(pk_bytes), &wallets[active_set[0]-1].session_pk);

    qs_compute_challenge(challenge_msg.challenge, 
                         &challenge_msg.w_agg_rounded, 
                         pk_bytes, sizeof(pk_bytes), 
                         msg, msglen);
    printf("\nBROKER: Aggregated Commitments. Challenge Hash: %02x%02x%02x%02x...\n", 
           challenge_msg.challenge[0], challenge_msg.challenge[1], 
           challenge_msg.challenge[2], challenge_msg.challenge[3]);

    /* ---------------------------------------------------------
     * 5. MSG_ROUND2_CHALL / MSG_ROUND3_RESP (BROKER -> WALLETS)
     *--------------------------------------------------------- */
    printf("\nBROKER: Broadcasting MSG_ROUND2_CHALL...\n");
    msg_response_t responses[T];
    polyvec_l_t z_shares[T];

    for (int i = 0; i < T; i++) {
        int w_idx = active_set[i] - 1;
        printf("  -> WALLET %d: Validating Challenge & Computing z_share...\n", wallets[w_idx].id);
        int rc = wallet_sign_round3(&wallets[w_idx], &challenge_msg, &responses[i]);
        if (rc != 0) {
            printf("  -> WALLET %d ERROR: code %d\n", wallets[w_idx].id, rc);
            return 1;
        }
        polyvec_l_copy(&z_shares[i], &responses[i].z_share);
    }

    /* ---------------------------------------------------------
     * 6. BROKER LOGIC: COMBINE RESPONSES
     *--------------------------------------------------------- */
    printf("\nBROKER: Received all z_shares. Combining MSG_SIGN_DONE...\n");
    
    poly_t c_poly;
    qs_expand_challenge(&c_poly, challenge_msg.challenge);

    msg_signature_t final_sig;
    memcpy(final_sig.challenge, challenge_msg.challenge, 32);

    qs_sign_combine(&final_sig.z_final, 
                    &final_sig.h_final, 
                    z_shares, 
                    m_rows_from_commits, 
                    T, 
                    &A, 
                    &wallets[active_set[0]-1].session_pk, 
                    &c_poly, 
                    &challenge_msg.w_agg_rounded);

    printf("\n=========================================================\n");
    printf("BROKER: Final Signature successfully generated!\n");
    
    /* ---------------------------------------------------------
     * 7. VERIFICATION
     *--------------------------------------------------------- */
    printf("VERIFIER: Calling wallet_verify on Node 5...\n");
    int valid = wallet_verify(&wallets[4], msg, msglen, ctr, &final_sig);
    
    if (valid) {
        printf("VERIFIER: [SUCCESS] Signature is MATCHING and VALID!\n");
    } else {
        printf("VERIFIER: [FAIL] Signature was REJECTED!\n");
        return 1;
    }

    printf("=========================================================\n");
    return 0;
}
