#ifndef QS_POLYVEC_H
#define QS_POLYVEC_H

#include "../common/prng.h"
#include "matrix.h"
#include "poly.h"

/* polyvec_l operations */
void polyvec_l_zero(polyvec_l_t *v);
void polyvec_l_copy(polyvec_l_t *dst, const polyvec_l_t *src);
void polyvec_l_add(polyvec_l_t *r, const polyvec_l_t *a, const polyvec_l_t *b);
void polyvec_l_sub(polyvec_l_t *r, const polyvec_l_t *a, const polyvec_l_t *b);
void polyvec_l_mul_scalar(polyvec_l_t *v, int64_t scalar);
int  polyvec_l_equal(const polyvec_l_t *a, const polyvec_l_t *b);
void polyvec_l_uniform(polyvec_l_t *v, qs_prng_t *prng);
void polyvec_l_mul_poly(polyvec_l_t *out, const poly_t *c, const polyvec_l_t *v);
void polyvec_l_sample_raccoon(polyvec_l_t *v, qs_prng_t *prng);  /* uniform in [-2^nu_w, 2^nu_w] */

/* polyvec_k operations */
void polyvec_k_zero(polyvec_k_t *v);
void polyvec_k_copy(polyvec_k_t *dst, const polyvec_k_t *src);
void polyvec_k_add(polyvec_k_t *r, const polyvec_k_t *a, const polyvec_k_t *b);
void polyvec_k_sub(polyvec_k_t *r, const polyvec_k_t *a, const polyvec_k_t *b);
int  polyvec_k_equal(const polyvec_k_t *a, const polyvec_k_t *b);
void polyvec_k_uniform(polyvec_k_t *v, qs_prng_t *prng);
void polyvec_k_mul_poly(polyvec_k_t *out, const poly_t *c, const polyvec_k_t *v);
void polyvec_k_shift_left(polyvec_k_t *out, const polyvec_k_t *in, int shift);
int64_t polyvec_k_norm_inf(const polyvec_k_t *v);
int64_t polyvec_l_norm_inf(const polyvec_l_t *v);
void polyvec_k_round_nuw(polyvec_k_t *out, const polyvec_k_t *in);

#endif /* QS_POLYVEC_H */
