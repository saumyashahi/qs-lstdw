#include "sign.h"
#include "../lattice/matrix.h"
#include "../lattice/polyvec.h"
#include "../common/prng.h"
#include "hash.h"
#include <string.h>

/*
Commit phase

r ← random
e' ← random
w = A*r + e'
*/

void signing_commit(
    qs_commit_share *out,
    const uint8_t *sid,
    const uint8_t *msg,
    const matrix_t *A,
    qs_prng_t *prng
)
{
    uint8_t buf[128];

    /* r_jk ← random L-vector */
    polyvec_l_uniform(&out->r, prng);

    /* e'_jk ← random K-vector */
    polyvec_k_uniform(&out->e_prime, prng);

    /* w_jk = A * r_jk */
    matrix_vec_mul(&out->w, A, &out->r);

    /* w_jk = A*r + e' */
    polyvec_k_add(&out->w, &out->w, &out->e_prime);

    /* hash commitment */

    memcpy(buf, sid, 32);

    size_t mlen = strlen((char*)msg);
    memcpy(buf + 32, msg, mlen);

    H(out->commitment, buf, 32 + mlen);

    /* row blinder */

    polyvec_l_uniform(&out->row_blinder, prng);
}