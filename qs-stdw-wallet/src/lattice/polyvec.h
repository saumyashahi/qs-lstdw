#ifndef QS_POLYVEC_H
#define QS_POLYVEC_H

#include "../common/prng.h"
#include "matrix.h"
#include "poly.h"

/* polyvec_l operations */

void polyvec_l_zero(polyvec_l_t *v);

void polyvec_l_copy(polyvec_l_t *dst,
                    const polyvec_l_t *src);

void polyvec_l_add(polyvec_l_t *r,
                   const polyvec_l_t *a,
                   const polyvec_l_t *b);

void polyvec_l_sub(polyvec_l_t *r,
                   const polyvec_l_t *a,
                   const polyvec_l_t *b);

void polyvec_l_mul_scalar(polyvec_l_t *v,
                          int32_t scalar);

int polyvec_l_equal(const polyvec_l_t *a,
                    const polyvec_l_t *b);


void polyvec_l_uniform(polyvec_l_t *v, 
                        qs_prng_t *prng);
#endif