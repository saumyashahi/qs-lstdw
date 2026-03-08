#ifndef QS_MESSAGE_H
#define QS_MESSAGE_H

#include <stdint.h>
#include <stddef.h>
#include "../lattice/polyvec.h"
#include "../signing/sign.h"

/*
 * Network messages for QS-STDW Threshold Signing
 */

/* Message types for the simulation broker */
typedef enum {
    MSG_SIGN_INIT,      /* Broker -> Wallets: start signing a message */
    MSG_ROUND1_COMMIT,  /* Wallet -> Broker: Round 1 commitment share */
    MSG_ROUND2_CHALL,   /* Broker -> Wallets: Aggregated w & challenge */
    MSG_ROUND3_RESP,    /* Wallet -> Broker: Round 3 response share (z_{j,k}) */
    MSG_SIGN_DONE       /* Broker -> Caller: Final signature */
} qs_msg_type_t;

/* Round 1: Commitment payload sent by each active signer */
typedef struct {
    uint32_t sender_id;               /* 1-based ID of the signer */
    qs_commit_share commit_share;     /* The full commit share (w, cmt, m_row, etc.) */
} msg_commit_t;

/* Round 2: Challenge payload broadcasted by the broker to all active signers */
typedef struct {
    uint8_t challenge[32];            /* The Fiat-Shamir challenge hash c_j */
    polyvec_k_t w_agg_rounded;        /* The aggregated, rounded commitments w_j */
} msg_challenge_t;

/* Round 3: Response payload sent by each active signer */
typedef struct {
    uint32_t sender_id;               /* 1-based ID of the signer */
    polyvec_l_t z_share;              /* The response share z_{j,k} */
} msg_response_t;

/* Final Signature produced by the broker */
typedef struct {
    uint8_t challenge[32];            /* c_j */
    polyvec_l_t z_final;              /* z_j */
    polyvec_k_t h_final;              /* h_j */
} msg_signature_t;

#endif /* QS_MESSAGE_H */
