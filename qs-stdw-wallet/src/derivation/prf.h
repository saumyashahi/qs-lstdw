#ifndef QS_PRF_H
#define QS_PRF_H

#include <stdint.h>
#include "../polyvec.h"

void qs_prf_row_blinder(
    polyvec *out,
    const uint8_t seed[32],
    const uint8_t sid[32]
);

void qs_prf_col_blinder(
    polyvec *out,
    const uint8_t seed[32],
    const uint8_t sid[32]
);

#endif