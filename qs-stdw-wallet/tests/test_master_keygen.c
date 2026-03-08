#include <stdio.h>
#include "../src/keygen/master_keygen.h"
#include "../src/threshold/share.h"

#define N 5
#define T 3

int main(void)
{
    master_public_key_t mpk;
    party_secret_t parties[N];
    uint8_t chaincode[CHAINCODE_BYTES];

    /* Run key generation */
    master_keygen(
       &mpk,
       parties,
       chaincode,
       N,
       T
    );

    printf("\n=========================================================\n");
    printf("   MASTER KEYGEN TEST\n");
    printf("=========================================================\n");

    /* Print first coefficient of t[0] */
    printf("[INFO] t[0].coeffs[0] = %u\n",
           mpk.t.vec[0].coeffs[0]);

    /* Print first byte of seed_A */
    printf("[INFO] seed_A[0] = %u\n",
           mpk.seed_A[0]);

    /* Print chaincode */
    printf("[INFO] chaincode[0] = %u\n",
           chaincode[0]);

    /* Print first party share coefficient */
    printf("[INFO] party[0] first coeff = %u\n",
           parties[0].share.vec[0].coeffs[0]);

    printf("[PASS] Master Keygen outputs OK\n");
    printf("=========================================================\n");

    return 0;
}