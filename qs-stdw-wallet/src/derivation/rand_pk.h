#ifndef RAND_PK_H
#define RAND_PK_H

#include "../lattice/polyvec.h"
#include "../lattice/matrix.h"
#include <stdint.h>

typedef struct {
    matrix_t A;
    polyvec_k_t t;
} public_key_t;

typedef struct {
    matrix_t A;
    polyvec_k_t t_prime;
} session_pk_t;

void derive_session_pk(
    session_pk_t *out,
    const public_key_t *mpk,
    const uint8_t *seed,
    const uint8_t *session_id
);

#endif