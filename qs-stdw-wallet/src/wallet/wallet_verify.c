#include "wallet.h"
#include "../verify/verify.h"
#include "../common/hash.h"
#include "../lattice/sample.h"
#include <string.h>

int wallet_verify(qs_wallet_t *w,
                  const uint8_t *msg,
                  size_t msglen,
                  uint32_t ctr,
                  const msg_signature_t *sig)
{
    if (!w || !msg || !sig) return 0;

    /* 1. sid = H(msg || ctr) */
    uint8_t sid[SESSION_ID_BYTES];
    HSessionID(sid, msg, msglen, ctr);

    /* 2. rho = H(chaincode || sid) */
    uint8_t rho_buf[CHAINCODE_BYTES + SESSION_ID_BYTES];
    memcpy(rho_buf,                   w->chaincode, CHAINCODE_BYTES);
    memcpy(rho_buf + CHAINCODE_BYTES, sid,          SESSION_ID_BYTES);
    uint8_t rho[32];
    H(rho, rho_buf, sizeof(rho_buf));

    /*
     * 3. Reconstruct the x^0 constant term of f_j.
     * f_j(0) = 0 since f_j has no constant term (only x^1, x^2, ...).
     * Therefore session_pk = t_master + A*0 = t_master.
     *
     * The wallet_sign_round1 side also sets session_pk = t_master + A*0.
     * Both sides must agree — which they do.
     */
    polyvec_k_t expected_session_pk;
    polyvec_k_copy(&expected_session_pk, w->t_master);

    /* 7-byte serialization for 49-bit q */
    uint8_t pk_bytes[QS_K * QS_N * 7];
    polyvec_k_to_bytes(pk_bytes, sizeof(pk_bytes), &expected_session_pk);

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
