#ifndef QS_POLY_FROM_SEED_H
#define QS_POLY_FROM_SEED_H

#include <stdint.h>
#include "../../config/params.h"

/*
 * Deterministically expand a seed into a polynomial
 * in R_q with coefficients derived via SHAKE256.
 */
void poly_from_seed(int32_t a[QS_N],
                    const uint8_t seed[SEED_BYTES]);

#endif