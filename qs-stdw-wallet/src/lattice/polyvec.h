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

/* polyvec_l additional */

/*
 * Multiply each component of v by polynomial c (in R_q)
 * out[i] = poly_mul(c, v[i])  for i in [L]
 */
void polyvec_l_mul_poly(polyvec_l_t *out,
                        const poly_t *c,
                        const polyvec_l_t *v);

/* Sample ephemeral vector with coefficients in [-(GAMMA1-1), GAMMA1] */
void polyvec_l_sample_gamma1(polyvec_l_t *v, qs_prng_t *prng);

/* polyvec_k operations */

void polyvec_k_zero(polyvec_k_t *v);

void polyvec_k_copy(polyvec_k_t *dst,
                    const polyvec_k_t *src);

void polyvec_k_add(polyvec_k_t *r,
                   const polyvec_k_t *a,
                   const polyvec_k_t *b);

void polyvec_k_sub(polyvec_k_t *r,
                   const polyvec_k_t *a,
                   const polyvec_k_t *b);

int polyvec_k_equal(const polyvec_k_t *a,
                    const polyvec_k_t *b);

void polyvec_k_uniform(polyvec_k_t *v,
                       qs_prng_t *prng);

/*
 * Multiply each component of v by polynomial c (in R_q)
 * out[i] = poly_mul(c, v[i])  for i in [K]
 */
void polyvec_k_mul_poly(polyvec_k_t *out,
                        const poly_t *c,
                        const polyvec_k_t *v);

/*
 * Scale each coefficient by 2^shift (left-shift) then reduce mod Q
 * Used for 2^{nu_t} * c * t'_pk
 */
void polyvec_k_shift_left(polyvec_k_t *out,
                          const polyvec_k_t *in,
                          int shift);

/* Compute infinity norm of polyvec_k (centered representative) */
int32_t polyvec_k_norm_inf(const polyvec_k_t *v);

/* Compute infinity norm of polyvec_l (centered representative) */
int32_t polyvec_l_norm_inf(const polyvec_l_t *v);

/* Round every coefficient: out[i] := round_nuw(in[i]) = in[i] >> NU_W */
void polyvec_k_round_nuw(polyvec_k_t *out, const polyvec_k_t *in);

#endif