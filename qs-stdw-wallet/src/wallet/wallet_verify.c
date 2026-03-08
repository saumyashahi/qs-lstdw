#include "wallet.h"
#include "../verify/verify.h"
#include "../common/hash.h"
#include "../lattice/sample.h"
#include <string.h>

/*
 * Verify a completed signature.
 * This is a clean wrapper around qs_verify, using the wallet's stored parameters.
 * Critically, a Verifier must deterministically derive the session public key 
 * mathematically from the master public key and the chaincode.
 */
int wallet_verify(qs_wallet_t *w,
                  const uint8_t *msg,
                  size_t msglen,
                  uint32_t ctr,
                  const msg_signature_t *sig)
{
    if (!w || !msg || !sig) return 0;

    /* 1. Session ID: sid = H(msg || ctr) */
    uint8_t sid[SESSION_ID_BYTES];
    HSessionID(sid, msg, msglen, ctr);

    /* 2. rho = H(chaincode || sid) */
    uint8_t rho_buf[CHAINCODE_BYTES + SESSION_ID_BYTES];
    memcpy(rho_buf, w->chaincode, CHAINCODE_BYTES);
    memcpy(rho_buf + CHAINCODE_BYTES, sid, SESSION_ID_BYTES);
    uint8_t rho[32];
    H(rho, rho_buf, sizeof(rho_buf));

    /* 3. Generate f_j(0) polynomial with small (ETA-bounded) coefficients */
    qs_prng_t fj_prng;
    prng_init(&fj_prng, rho);
    
    polyvec_l_t fj0;
    polyvec_l_zero(&fj0);
    /* We only need the x^0 term for the public key (d=0) */
    for (int i = 0; i < QS_L; i++) {
        sample_small_poly(&fj0.vec[i], &fj_prng);
    }
    
    /* 4. RandPK: t'_pk = t_master + A * f_j(0) */
    polyvec_k_t expected_session_pk;
    polyvec_k_t Af0;
    matrix_vec_mul(&Af0, w->A, &fj0);
    polyvec_k_add(&expected_session_pk, w->t_master, &Af0);

    uint8_t pk_bytes[QS_K * QS_N * 4];
    polyvec_k_to_bytes(pk_bytes, sizeof(pk_bytes), &expected_session_pk);

    /* 5. The original message the verifier checks must match */
    return qs_verify(sig->challenge,
                     &sig->z_final,
                     &sig->h_final,
                     &expected_session_pk,
                     pk_bytes,
                     sizeof(pk_bytes),
                     msg,
                     msglen,
                     w->A);
}
