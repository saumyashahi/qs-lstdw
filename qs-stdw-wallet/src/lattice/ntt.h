#ifndef QS_NTT_H
#define QS_NTT_H

#include "poly.h"
#include "polyvec.h"

/* Forward NTT: coefficient domain -> NTT domain */
void poly_ntt(poly_t *a);

/* Inverse NTT: NTT domain -> coefficient domain */
void poly_invntt(poly_t *a);

/* Pointwise multiplication in NTT domain (both inputs must be in NTT domain) */
void poly_pointwise_mul(poly_t *r,
                        const poly_t *a,
                        const poly_t *b);

/* Vector transforms */
void polyvec_l_ntt(polyvec_l_t *v);
void polyvec_l_invntt(polyvec_l_t *v);
void polyvec_k_ntt(polyvec_k_t *v);
void polyvec_k_invntt(polyvec_k_t *v);

#endif /* QS_NTT_H */
