#ifndef QS_POLY_FROM_SEED_H
#define QS_POLY_FROM_SEED_H

#include <stdint.h>
#include "../../config/params.h"

/*
 * Deterministically expand a seed into a polynomial in R_q.
 * Coefficients are in [0, q-1] where q = RACCOON_Q (49-bit composite).
 */
void poly_from_seed(int64_t a[QS_N], const uint8_t seed[SEED_BYTES]);

#endif
