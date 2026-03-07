#ifndef SHAMIR_H
#define SHAMIR_H

#include "../lattice/matrix.h"
#include "../common/prng.h"
#include "share.h"

void shamir_split(const polyvec_l_t *secret,
                  shamir_share_t shares[],
                  int n,
                  int t,
                  qs_prng_t *prng);

void shamir_reconstruct(polyvec_l_t *result,
                        const shamir_share_t shares[],
                        int t);

#endif
