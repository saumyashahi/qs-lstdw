#include "wallet.h"
#include <string.h>

/*
 * Initialize a wallet instance with its Shamir share and global parameters.
 */
void wallet_init(qs_wallet_t *w,
                 uint32_t id,
                 const polyvec_l_t *share,
                 const uint8_t pairwise_seeds[MAX_PARTIES][32],
                 const uint8_t chaincode[CHAINCODE_BYTES],
                 matrix_t *A,
                 polyvec_k_t *t_master,
                 int N,
                 int T,
                 const uint8_t *entropy_seed)
{
    memset(w, 0, sizeof(qs_wallet_t));

    w->id = id;
    polyvec_l_copy(&w->master_share, share);

    for (int i = 0; i < N; i++) {
        memcpy(w->pairwise_seeds[i], pairwise_seeds[i], 32);
    }
    
    memcpy(w->chaincode, chaincode, CHAINCODE_BYTES);

    w->A = A;
    w->t_master = t_master;
    w->total_parties = N;
    w->threshold = T;

    /* Initialize inner PRNG with unique entropy */
    uint8_t prng_seed[32];
    if (entropy_seed) {
        memcpy(prng_seed, entropy_seed, 32);
    } else {
        /* Deterministic fallback for testing if no entropy provided */
        memset(prng_seed, (uint8_t)id, 32);
    }
    prng_init(&w->prng, prng_seed);

    /* Clear active session state */
    w->active_count = 0;
    w->active_idx = -1;
}
