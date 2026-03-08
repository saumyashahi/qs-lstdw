#ifndef QS_VERIFY_H
#define QS_VERIFY_H

#include <stdint.h>
#include <stddef.h>

#include "../lattice/matrix.h"
#include "../lattice/polyvec.h"

/*
 * Algorithm 5 — QS-STDW.Verify
 *
 * Checks a signature σ_j = (c_j, z_j, h_j) against:
 *   - rerandomized public key pk_j = (A, t'_{pk_j})
 *   - pk_bytes: serialization of t'_{pk_j} (for Hc input)
 *   - message m
 *
 * Returns 1 if signature is valid, 0 otherwise.
 */
int qs_verify(
    const uint8_t challenge[32],   /* c_j */
    const polyvec_l_t *z,          /* z_j */
    const polyvec_k_t *h,          /* h_j */
    const polyvec_k_t *t_pk,       /* t'_{pk_j}: rerandomized trapdoor */
    const uint8_t *pk_bytes,       /* serialized t'_pk for Hc */
    size_t pk_bytes_len,
    const uint8_t *msg,
    size_t msglen,
    const matrix_t *A
);

#endif /* QS_VERIFY_H */