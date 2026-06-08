#include "sign.h"
#include "../../config/params.h"

/*
 * Lagrange interpolation coefficient for Shamir secret reconstruction.
 *
 * λ_{act,k} = Π_{j ∈ act, j ≠ k} [ j · (j - k)^{-1} ]  mod q
 *
 * All arithmetic is mod RACCOON_Q.
 */

static int64_t modinv_q(int64_t a)
{
    int64_t q   = RACCOON_Q;
    int64_t t   = 0, newt = 1;
    int64_t r   = q,  newr = ((a % q) + q) % q;

    while (newr != 0) {
        int64_t quo = r / newr;
        int64_t tmp;
        tmp = newt;  newt = t - quo * newt;  t = tmp;
        tmp = newr;  newr = r - quo * newr;  r = tmp;
    }
    if (r > 1) return 0;   /* not invertible */
    if (t < 0) t += q;
    return t;
}

int64_t qs_lagrange_coeff_modq(int signer_id, const int *active_set, int t)
{
    __int128 num = 1;
    __int128 den = 1;
    __int128 Q   = RACCOON_Q;

    for (int i = 0; i < t; i++) {
        int j = active_set[i];
        if (j == signer_id) continue;

        num = (num * j) % Q;
        if (num < 0) num += Q;

        __int128 diff = ((int64_t)j - signer_id) % (int64_t)Q;
        if (diff < 0) diff += Q;
        den = (den * diff) % Q;
    }

    int64_t den_inv = modinv_q((int64_t)(den % Q));
    __int128 lambda = (num * den_inv) % Q;
    if (lambda < 0) lambda += Q;
    return (int64_t)lambda;
}
