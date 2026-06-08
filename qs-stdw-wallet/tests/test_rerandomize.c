#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "../src/keygen/master_keygen.h"
#include "../src/threshold/rerandomize.h"
#include "../config/params.h"

static int64_t mod_q(int64_t x)
{
    x %= RACCOON_Q;
    if (x < 0) x += RACCOON_Q;
    return x;
}

static int64_t mulmod(int64_t a, int64_t b)
{
    __int128 r = (__int128)a * b;
    int64_t  m = (int64_t)(r % RACCOON_Q);
    if (m < 0) m += RACCOON_Q;
    return m;
}

static int64_t modinv(int64_t a)
{
    int64_t t = 0, newt = 1;
    int64_t r = RACCOON_Q, newr = ((a % RACCOON_Q) + RACCOON_Q) % RACCOON_Q;
    while (newr) {
        int64_t q = r / newr, tmp;
        tmp = newt; newt = t - q * newt; t = tmp;
        tmp = newr; newr = r - q * newr; r = tmp;
    }
    assert(r == 1);
    if (t < 0) t += RACCOON_Q;
    return t;
}

/* Lagrange interpolation at x=0, mod RACCOON_Q */
static int64_t reconstruct(party_secret_t *parties, int T)
{
    int64_t secret = 0;
    for (int i = 0; i < T; i++) {
        int64_t xi = parties[i].id;
        int64_t li  = 1;
        for (int j = 0; j < T; j++) {
            if (i == j) continue;
            int64_t xj  = parties[j].id;
            int64_t num = mod_q(-xj);
            int64_t den = mod_q(xi - xj);
            li = mulmod(li, mulmod(num, modinv(den)));
        }
        secret = mod_q(secret + mulmod(parties[i].share.vec[0].coeffs[0], li));
    }
    return secret;
}

int main(void)
{
    int N = 5, T = 3;
    party_secret_t parties[5];
    master_public_key_t mpk;
    uint8_t chaincode[CHAINCODE_BYTES];

    master_keygen(&mpk, parties, chaincode, N, T);

    printf("\n=========================================================\n");
    printf("   RERANDOMIZATION TEST (Raccoon-128)\n");
    printf("=========================================================\n");

    int64_t original = reconstruct(parties, T);
    printf("[INFO] Original reconstructed secret = %lld\n", (long long)original);

    party_secret_t parties_copy[5];
    memcpy(parties_copy, parties, sizeof(parties));

    uint8_t session_id[8] = {1,2,3,4,5,6,7,8};
    rerandomize_shares(parties_copy, N, T, parties[0].chaincode,
                       session_id, sizeof(session_id));

    int64_t after = reconstruct(parties_copy, T);
    printf("[INFO] After rerandomization secret = %lld\n", (long long)after);

    if (original != after) {
        printf("[FAIL] ERROR: secret changed! %lld -> %lld\n",
               (long long)original, (long long)after);
        return 1;
    }
    printf("[PASS] Secret invariant verified\n");

    int changed = 0;
    for (int i = 0; i < N; i++) {
        if (parties[i].share.vec[0].coeffs[0] !=
            parties_copy[i].share.vec[0].coeffs[0])
            changed = 1;
    }
    if (!changed) {
        printf("[FAIL] ERROR: shares did not change!\n");
        return 1;
    }
    printf("[PASS] Shares successfully rerandomized\n");
    printf("=========================================================\n");
    return 0;
}
