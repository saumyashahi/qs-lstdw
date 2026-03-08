#ifndef QS_WALLET_H
#define QS_WALLET_H

#include <stdint.h>
#include <stddef.h>

#include "../common/prng.h"
#include "../lattice/matrix.h"
#include "../lattice/polyvec.h"
#include "../config/params.h"
#include "../threshold/share.h"
#include "../signing/sign.h"
#include "../net/message.h"

/*
 * High-level Wallet Abstraction
 * Encapsulates a single party's state in the QS-STDW protocol.
 */

typedef struct {
    /* Identity & Secrets */
    uint32_t id;                                /* 1-based party ID */
    polyvec_l_t master_share;                   /* s_k: master secret share */
    uint8_t pairwise_seeds[MAX_PARTIES][32];    /* Symmetric pairwise PRF seeds */
    uint8_t chaincode[CHAINCODE_BYTES];         /* Master chaincode for derivation */

    /* Public Parameters */
    matrix_t *A;                                /* Global public matrix A */
    polyvec_k_t *t_master;                      /* Global master public key t */
    int total_parties;                          /* N */
    int threshold;                              /* T */

    /* State for currently active signing session */
    uint8_t active_session_id[SESSION_ID_BYTES];/* sid = H(msg || ctr) */
    uint8_t active_msg[256];                    /* Message being signed */
    size_t  active_msg_len;
    
    int active_set[MAX_PARTIES];                /* 1-based IDs of active signers */
    int active_count;                           /* Number of active signers (T) */
    int active_idx;                             /* This wallet's 0-based index in active_set */

    /* Rerandomized keys for the active session */
    polyvec_k_t session_pk;                     /* t'_{pk_j} */
    uint8_t session_pk_bytes[QS_K * QS_N * 4];  /* Serialized session public key */
    polyvec_l_t session_sk_share;               /* sk_{j,k} = s_k + f_j(k) */

    /* Round 1 state to keep for Round 3 */
    qs_commit_share current_commit;             /* Holds r, e', w, blinders */

    /* Randomness */
    qs_prng_t prng;

} qs_wallet_t;

/* Initialize a wallet instance with its Shamir share and global parameters */
void wallet_init(qs_wallet_t *w,
                 uint32_t id,
                 const polyvec_l_t *share,
                 const uint8_t pairwise_seeds[MAX_PARTIES][32],
                 const uint8_t chaincode[CHAINCODE_BYTES],
                 matrix_t *A,
                 polyvec_k_t *t_master,
                 int N,
                 int T,
                 const uint8_t *entropy_seed);

/* 
 * Start a signing session. 
 * Derives the session ID, rerandomizes the PK and SK share, and produces Round 1 commit.
 */
int wallet_sign_round1(qs_wallet_t *w,
                       const uint8_t *msg,
                       size_t msglen,
                       uint32_t ctr,
                       const int *active_set,
                       int active_count,
                       msg_commit_t *out_commit);

/*
 * Process Round 2 Challenge from the broker.
 * Computes and returns the Round 3 response share (z).
 */
int wallet_sign_round3(qs_wallet_t *w,
                       const msg_challenge_t *in_challenge,
                       msg_response_t *out_response);

/*
 * Verify a completed signature. This is a wrapper around the core qs_verify function.
 */
int wallet_verify(qs_wallet_t *w,
                  const uint8_t *msg,
                  size_t msglen,
                  uint32_t ctr,
                  const msg_signature_t *sig);

#endif /* QS_WALLET_H */
