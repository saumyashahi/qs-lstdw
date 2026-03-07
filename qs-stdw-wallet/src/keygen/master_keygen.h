#ifndef MASTER_KEYGEN_H
#define MASTER_KEYGEN_H

#include <stdint.h>
#include "lattice/poly.h"
#include "lattice/matrix.h"
#include "../threshold/share.h"

#define SEED_BYTES 32
#define QS_L 4
#define QS_K 4

typedef struct {
    matrix_t A;
    polyvec_k_t t;
    uint8_t seed_A[SEED_BYTES];
} master_public_key_t;

void master_keygen(master_public_key_t *mpk,
                   party_secret_t parties[MAX_PARTIES],
                   uint8_t chaincode[CHAINCODE_BYTES],
                   int N,
                   int T);

#endif