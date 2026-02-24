#include <stdlib.h>
#include "poly.h"

/* Ensure coefficient is in [0, Q-1] */
static inline int32_t mod_q(int64_t x)
{
    x %= Q;
    if (x < 0)
        x += Q;
    return (int32_t)x;
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
    for (int i = 0; i < QS_N; i++) {
        int64_t sum = (int64_t)a->coeffs[i] + b->coeffs[i];
        r->coeffs[i] = mod_q(sum);
    }
}

void poly_sub(poly_t *r, const poly_t *a, const poly_t *b)
{
    for (int i = 0; i < QS_N; i++) {
        int64_t diff = (int64_t)a->coeffs[i] - b->coeffs[i];
        r->coeffs[i] = mod_q(diff);
    }
}

void poly_zero(poly_t *a) {
    for (int i = 0; i < QS_N; i++) {
        a->coeffs[i] = 0;
    }
}

int32_t poly_norm(const poly_t *a) {
    return poly_norm_inf(a);
}

/*
 * Schoolbook negacyclic multiplication:
 *
 * (a * b) mod (x^QS_N + 1)
 *
 * When degree >= QS_N:
 *   x^QS_N ≡ -1
 */
void poly_mul(poly_t *r, const poly_t *a, const poly_t *b)
{
    int64_t tmp[2 * QS_N] = {0};

    /* Standard convolution */
    for (int i = 0; i < QS_N; i++) {
        for (int j = 0; j < QS_N; j++) {
            tmp[i + j] += (int64_t)a->coeffs[i] * b->coeffs[j];
        }
    }

    /* Negacyclic reduction: x^QS_N = -1 */
    for (int i = QS_N; i < 2 * QS_N - 1; i++) {
        tmp[i - QS_N] -= tmp[i];
    }

    /* Final reduction mod Q */
    for (int i = 0; i < QS_N; i++) {
        r->coeffs[i] = mod_q(tmp[i]);
    }
}

int32_t poly_norm_inf(const poly_t *a)
{
    int32_t max = 0;

    for (int i = 0; i < QS_N; i++) {

        int32_t x = a->coeffs[i];

        /* Convert to centered representative in (-Q/2, Q/2] */
        if (x > Q/2)
            x -= Q;

        int32_t absx = x < 0 ? -x : x;

        if (absx > max)
            max = absx;
    }

    return max;
}