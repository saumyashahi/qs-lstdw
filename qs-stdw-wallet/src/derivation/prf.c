#include "prf.h"
#include "../hash.h"
#include "../polyvec.h"

void qs_prf_row_blinder(
    polyvec *out,
    const uint8_t seed[32],
    const uint8_t sid[32]
)
{
    uint8_t input[64];
    uint8_t buf[POLYVEC_BYTES];

    memcpy(input, seed, 32);
    memcpy(input + 32, sid, 32);

    shake256(buf, sizeof(buf), input, sizeof(input));

    polyvec_frombytes(out, buf);
}

void qs_prf_col_blinder(
    polyvec *out,
    const uint8_t seed[32],
    const uint8_t sid[32]
)
{
    uint8_t input[65];
    uint8_t buf[POLYVEC_BYTES];

    memcpy(input, seed, 32);
    memcpy(input + 32, sid, 32);
    input[64] = 1;   /* domain separation */

    shake256(buf, sizeof(buf), input, sizeof(input));

    polyvec_frombytes(out, buf);
}