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
#include "../src/threshold/rerandomize.h"
#include "../src/net/message.h"
#include "../src/lattice/polyvec.h"
#include "../src/signing/challenge_poly.h"
#include "../src/derivation/rand_pk.h"

static double diff_ms(struct timespec s, struct timespec e) {
    return (e.tv_sec - s.tv_sec)*1000.0 + (e.tv_nsec - s.tv_nsec)/1000000.0;
}

int main(void) {
    int thresholds[] = {4, 16, 64, 256, 1024};
    int num_tests = 5;

    printf("  T    KeyGen   RandSK   RandPK   SS_1          SS_2          SS_3          Combine   Verify\n");
    printf("------------------------------------------------------------------------------------------------\n");

    for (int t_idx = 0; t_idx < num_tests; t_idx++) {
        int T = thresholds[t_idx];
        int N = T;

        party_secret_t *parties     = malloc(N * sizeof(party_secret_t));
        party_secret_t *rand_shares = malloc(N * sizeof(party_secret_t));
        qs_wallet_t    *wallets     = malloc(N * sizeof(qs_wallet_t));
        int            *active_set  = malloc(T * sizeof(int));
        msg_commit_t   *commits     = malloc(T * sizeof(msg_commit_t));
        msg_response_t *responses   = malloc(T * sizeof(msg_response_t));

        if (!parties||!rand_shares||!wallets||!active_set||!commits||!responses) {
            printf("OOM at T=%d\n", T); return 1;
        }

        struct timespec ts, te;
        double t_kg, t_rsk, t_rpk, t_s1, t_s2, t_s3, t_cb, t_vf;

        uint8_t chaincode[32];
        master_public_key_t mpk;

        /* 1. KeyGen */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        master_keygen(&mpk, parties, chaincode, N, T);
        clock_gettime(CLOCK_MONOTONIC, &te);
        t_kg = diff_ms(ts, te);

        /* Build session_id once (used for both RandSK and RandPK) */
        uint8_t msg[]  = "TRANSFER 10.0 TO ADDRESS 0xXYZ";
        size_t  msglen = strlen((char*)msg);
        uint32_t ctr   = 42;
        uint8_t session_id[SESSION_ID_BYTES];
        HSessionID(session_id, msg, msglen, ctr);

        /* 2. RandSK – rerandomise all N shares for this session */
        memcpy(rand_shares, parties, N * sizeof(party_secret_t));
        clock_gettime(CLOCK_MONOTONIC, &ts);
        rerandomize_shares(rand_shares, N, T, chaincode,
                           session_id, SESSION_ID_BYTES);
        clock_gettime(CLOCK_MONOTONIC, &te);
        t_rsk = diff_ms(ts, te);

        /* Wallet init (original shares, unmodified) */
        for (int i = 0; i < N; i++) {
            uint8_t ent[32] = {0};
            wallet_init(&wallets[i], parties[i].id, &parties[i].share,
                        (const uint8_t(*)[32])parties[i].pairwise_seeds,
                        chaincode, &mpk.A, &mpk.t, N, T, ent);
            active_set[i] = parties[i].id;
        }

        /* 3. RandPK – single-party, T-independent */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        session_pk_t spk;
        derive_session_pk(&spk,
                          (const public_key_t*)&mpk,
                          wallets[0].pairwise_seeds[0],
                          session_id);
        clock_gettime(CLOCK_MONOTONIC, &te);
        t_rpk = diff_ms(ts, te);

        /* 4. ShareSign_1 */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int i = 0; i < T; i++)
            wallet_sign_round1(&wallets[i], msg, msglen, ctr,
                               active_set, T, &commits[i]);
        clock_gettime(CLOCK_MONOTONIC, &te);
        t_s1 = diff_ms(ts, te);

        /* 5. ShareSign_2 */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        polyvec_k_t w_agg; polyvec_k_zero(&w_agg);
        for (int i = 0; i < T; i++)
            polyvec_k_add(&w_agg, &w_agg, &commits[i].commit_share.w);
        polyvec_k_t w_agg_r; polyvec_k_round_nuw(&w_agg_r, &w_agg);
        msg_challenge_t ch;
        qs_compute_challenge(ch.challenge, &w_agg_r,
                             wallets[0].session_pk_bytes,
                             sizeof(wallets[0].session_pk_bytes),
                             msg, msglen);
        clock_gettime(CLOCK_MONOTONIC, &te);
        t_s2 = diff_ms(ts, te);

        /* 6. ShareSign_3 */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int i = 0; i < T; i++)
            wallet_sign_round3(&wallets[i], &ch, &responses[i]);
        clock_gettime(CLOCK_MONOTONIC, &te);
        t_s3 = diff_ms(ts, te);

        /* 7. Combine */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        msg_signature_t sig;
        memcpy(sig.challenge, ch.challenge, 32);
        polyvec_l_t *zs = malloc(T*sizeof(polyvec_l_t));
        polyvec_l_t *ms = malloc(T*sizeof(polyvec_l_t));
        for (int i=0;i<T;i++){
            polyvec_l_copy(&zs[i],&responses[i].z_share);
            polyvec_l_copy(&ms[i],&commits[i].commit_share.m_row);
        }
        poly_t cp; qs_expand_challenge(&cp, sig.challenge);
        qs_sign_combine(&sig.z_final, &sig.h_final,
                        zs, ms, T, &mpk.A, &wallets[0].session_pk,
                        &cp, &w_agg_r);
        free(zs); free(ms);
        clock_gettime(CLOCK_MONOTONIC, &te);
        t_cb = diff_ms(ts, te);

        /* 8. Verify */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        int ok = wallet_verify(&wallets[0], msg, msglen, ctr, &sig);
        clock_gettime(CLOCK_MONOTONIC, &te);
        t_vf = diff_ms(ts, te);

        if (!ok) printf("VERIFY FAILED T=%d\n", T);

        printf("%4d  %8.3f  %7.3f  %7.3f  %12.3f  %12.3f  %12.3f  %8.3f  %8.3f\n",
               T, t_kg, t_rsk, t_rpk, t_s1, t_s2, t_s3, t_cb, t_vf);

        free(parties); free(rand_shares); free(wallets);
        free(active_set); free(commits); free(responses);
    }
    return 0;
}