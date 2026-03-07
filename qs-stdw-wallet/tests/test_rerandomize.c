#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "../src/keygen/master_keygen.h"
#include "../src/threshold/rerandomize.h"

#define Q 8380417

/* basic mod q */
static int32_t mod_q(int64_t x) {
    int32_t r = x % Q;
    if (r < 0) r += Q;
    return r;
}

/* modular inverse using extended Euclid */
static int32_t modinv(int32_t a) {
    int32_t t = 0, newt = 1;
    int32_t r = Q, newr = a;

    while (newr != 0) {
        int32_t q = r / newr;

        int32_t tmp = newt;
        newt = t - q * newt;
        t = tmp;

        tmp = newr;
        newr = r - q * newr;
        r = tmp;
    }

    if (r > 1) {
        printf("Not invertible!\n");
        assert(0);
    }

    if (t < 0)
        t += Q;

    return t;
}

/* reconstruct secret using Lagrange interpolation at x=0 */
static int32_t reconstruct(party_secret_t *parties, int T) {
    int64_t secret = 0;

    for (int i = 0; i < T; i++) {
        int32_t xi = parties[i].id;
        int64_t li = 1;

        for (int j = 0; j < T; j++) {
            if (i == j) continue;

            int32_t xj = parties[j].id;

            int32_t num = mod_q(-xj);
            int32_t den = mod_q(xi - xj);

            int32_t inv = modinv(den);

            li = mod_q(li * num);
            li = mod_q(li * inv);
        }

        secret += (int64_t)parties[i].share.vec[0].coeffs[0] * li;
    }

    return mod_q(secret);
}

int main() {

    int N = 5;
    int T = 3;

    party_secret_t parties[N];

    master_public_key_t mpk;
    uint8_t chaincode[CHAINCODE_BYTES];

    master_keygen(
        &mpk,
        parties,
        chaincode,
        N,
        T
    );

    /* reconstruct original */
    int32_t original = reconstruct(parties, T);

    printf("Original reconstructed secret = %d\n", original);

    /* copy parties */
    party_secret_t parties_copy[N];
    memcpy(parties_copy, parties, sizeof(parties));

    /* rerandomize */
    uint8_t session_id[8] = {1,2,3,4,5,6,7,8};
    rerandomize_shares(parties_copy,
                       N,
                       T,
                       parties[0].chaincode,
                       session_id,
                       sizeof(session_id));

    /* reconstruct again */
    int32_t after = reconstruct(parties_copy, T);

    printf("After rerandomization secret = %d\n", after);

    /* verify invariant */
    if (original != after) {
        printf("ERROR: secret changed!\n");
        return 1;
    }

    printf("Secret invariant verified.\n");

    /* verify shares changed */
    int changed = 0;
    for (int i = 0; i < N; i++) {
        if (parties[i].share.vec[0].coeffs[0] !=
        parties_copy[i].share.vec[0].coeffs[0]) {
                changed = 1;
            }
    }

    if (!changed) {
        printf("ERROR: shares did not change!\n");
        return 1;
    }

    printf("Shares successfully rerandomized.\n");

    printf("TEST PASSED.\n");
    return 0;
}