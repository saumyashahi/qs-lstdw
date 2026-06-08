#include <stdlib.h>
#include "poly.h"
#include "ntt.h"

/* Reduce x into [0, q-1] */
static inline int64_t mod_q(int64_t x)
{
    x %= RACCOON_Q;
    if (x < 0) x += RACCOON_Q;
    return x;
}

void poly_copy(poly_t *r, const poly_t *a)
{
    for (int i = 0; i < QS_N; i++)
        r->coeffs[i] = a->coeffs[i];
}

void poly_reduce(poly_t *a)
{
    for (int i = 0; i < QS_N; i++)
        a->coeffs[i] = mod_q(a->coeffs[i]);
}

void poly_add(poly_t *r, const poly_t *a, const poly_t *b)
{
    for (int i = 0; i < QS_N; i++)
        r->coeffs[i] = mod_q(a->coeffs[i] + b->coeffs[i]);
}

void poly_sub(poly_t *r, const poly_t *a, const poly_t *b)
{
    for (int i = 0; i < QS_N; i++)
        r->coeffs[i] = mod_q(a->coeffs[i] - b->coeffs[i]);
}

void poly_zero(poly_t *a)
{
    for (int i = 0; i < QS_N; i++)
        a->coeffs[i] = 0;
}

int64_t poly_norm(const poly_t *a) { return poly_norm_inf(a); }

/* Negacyclic multiplication via NTT */
void poly_mul(poly_t *r, const poly_t *a, const poly_t *b)
{
    poly_t A, B;
    poly_copy(&A, a);
    poly_copy(&B, b);
    poly_ntt(&A);
    poly_ntt(&B);
    poly_pointwise_mul(&A, &A, &B);
    poly_invntt(&A);
    poly_copy(r, &A);
}

int64_t poly_norm_inf(const poly_t *a)
{
    int64_t max = 0;
    for (int i = 0; i < QS_N; i++) {
        int64_t x = a->coeffs[i];
        /* Centered representative in (-q/2, q/2] */
        if (x > RACCOON_Q / 2) x -= RACCOON_Q;
        if (x < 0) x = -x;
        if (x > max) max = x;
    }
    return max;
}
