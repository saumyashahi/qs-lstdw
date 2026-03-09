#ifndef SHARE_H
#define SHARE_H

#include "../lattice/matrix.h"
#include "../lattice/poly.h"
#include "../../config/params.h"
#include <stdint.h>

#define MAX_PARTIES 2048
#define SEED_BYTES 32
#define SIG_SK_BYTES 32
#define SIG_PK_BYTES 32
#define CHAINCODE_BYTES 32

typedef struct {

    uint32_t id;                // Shamir x-coordinate

    polyvec_l_t share;          // secret share s_i

    uint8_t chaincode[CHAINCODE_BYTES];

    uint8_t pairwise_seeds[MAX_PARTIES][SEED_BYTES];

    uint8_t sig_sk[SIG_SK_BYTES];
    uint8_t sig_pk[SIG_PK_BYTES];

} party_secret_t;

typedef struct {

    uint32_t id;
    polyvec_l_t value;

} shamir_share_t;

#endif