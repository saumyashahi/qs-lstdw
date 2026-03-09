#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include "../config/params.h"
#include "../src/common/hash.h"
#include "../src/keygen/master_keygen.h"
#include "../src/wallet/wallet.h"
#include "../src/threshold/share.h"
#include "../src/net/message.h"
#include "../src/lattice/polyvec.h"
#include "../src/signing/challenge_poly.h"

// Helper to get time diff in milliseconds
static double diff_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 + 
           (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

int main(void) {
    int thresholds[] = {4, 16, 64, 256, 1024};
    int num_tests = sizeof(thresholds) / sizeof(thresholds[0]);

    printf("  T    KeyGen    ShareSign_1    ShareSign_2    ShareSign_3    Combine    Verify\n");
    printf("--------------------------------------------------------------------------------\n");

    for (int t_idx = 0; t_idx < num_tests; t_idx++) {
        int T = thresholds[t_idx];
        int N = T; // Full consensus scenario

        /* Heap allocate to avoid stack crash at T=1024 */
        party_secret_t *parties = malloc(N * sizeof(party_secret_t));
        qs_wallet_t *wallets = malloc(N * sizeof(qs_wallet_t));
        int *active_set = malloc(T * sizeof(int));
        msg_commit_t *commits = malloc(T * sizeof(msg_commit_t));
        msg_response_t *responses = malloc(T * sizeof(msg_response_t));
        
        if (!parties || !wallets || !active_set || !commits || !responses) {
            printf("Memory allocation failed for T=%d\n", T);
            return -1;
        }

        struct timespec t_start, t_end;
        double time_keygen, time_sign1 = 0, time_sign2 = 0, time_sign3 = 0, time_combine = 0, time_verify = 0;
        
        uint8_t chaincode[32];
        master_public_key_t mpk;

        /* ========== 1. KEYGEN ========== */
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        master_keygen(&mpk, parties, chaincode, N, T);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        time_keygen = diff_ms(t_start, t_end);

        /* Wallet Initialization */
        for (int i = 0; i < N; i++) {
            uint8_t entropy_seed[32] = {0}; // Dummy entropy for deterministic testing
            wallet_init(&wallets[i], parties[i].id, &parties[i].share,
                        (const uint8_t (*)[32])parties[i].pairwise_seeds, chaincode, 
                        &mpk.A, &mpk.t, N, T, entropy_seed);
            active_set[i] = parties[i].id;
        }

        uint8_t msg[] = "TRANSFER 10.0 TO ADDRESS 0xXYZ";
        size_t msglen = strlen((char *)msg);
        uint32_t ctr = 42;

        /* ========== 2. SHARESIGN_1 (Commitments) ========== */
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        for (int i = 0; i < T; i++) {
            wallet_sign_round1(&wallets[i], msg, msglen, ctr, active_set, T, &commits[i]);
        }
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        time_sign1 = diff_ms(t_start, t_end);

        /* ========== 3. SHARESIGN_2 (Broker aggregation / Auth) ========== */
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        polyvec_k_t w_agg;
        polyvec_k_zero(&w_agg);
        for(int i = 0; i < T; i++) {
            polyvec_k_add(&w_agg, &w_agg, &commits[i].commit_share.w);
        }
        
        polyvec_k_t w_agg_rounded;
        polyvec_k_round_nuw(&w_agg_rounded, &w_agg);

        msg_challenge_t challenge_msg;
        uint8_t tmp_sid[SESSION_ID_BYTES];
        HSessionID(tmp_sid, msg, msglen, ctr);

        // Compute c = H(msg || w_agg)
        // We use wallets[0].session_pk_bytes for representation logic
        qs_compute_challenge(challenge_msg.challenge, &w_agg_rounded,
                             wallets[0].session_pk_bytes, sizeof(wallets[0].session_pk_bytes),
                             msg, msglen);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        time_sign2 = diff_ms(t_start, t_end);

        /* ========== 4. SHARESIGN_3 (Responses) ========== */
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        for (int i = 0; i < T; i++) {
            wallet_sign_round3(&wallets[i], &challenge_msg, &responses[i]);
        }
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        time_sign3 = diff_ms(t_start, t_end);

        /* ========== 5. COMBINE ========== */
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        msg_signature_t final_sig;
        memcpy(final_sig.challenge, challenge_msg.challenge, 32);

        polyvec_l_t *z_shares = malloc(T * sizeof(polyvec_l_t));
        polyvec_l_t *m_rows   = malloc(T * sizeof(polyvec_l_t));
        for (int i = 0; i < T; i++) {
            polyvec_l_copy(&z_shares[i], &responses[i].z_share);
            polyvec_l_copy(&m_rows[i], &commits[i].commit_share.m_row);
        }

        poly_t c_poly;
        qs_expand_challenge(&c_poly, final_sig.challenge);

        qs_sign_combine(&final_sig.z_final, &final_sig.h_final,
                        z_shares, m_rows, T,
                        &mpk.A, &wallets[0].session_pk,
                        &c_poly, &w_agg_rounded);
        
        free(z_shares);
        free(m_rows);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        time_combine = diff_ms(t_start, t_end);

        /* ========== 6. VERIFY ========== */
        // We evaluate verification natively
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        int valid = wallet_verify(&wallets[0], msg, msglen, ctr, &final_sig);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        time_verify = diff_ms(t_start, t_end);

        if (!valid) {
            printf("Verification FAILED at T=%d\n", T);
        }

        // Print table row
        printf("%4d  %8.3f  %13.3f  %13.3f  %13.3f  %9.3f  %9.3f\n",
               T, time_keygen, time_sign1, time_sign2, time_sign3, time_combine, time_verify);

        /* Cleanup */
        free(parties);
        free(wallets);
        free(active_set);
        free(commits);
        free(responses);
    }

    return 0;
}
