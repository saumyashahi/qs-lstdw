#include <stdint.h>
#include <string.h>
#include "../config/params.h"
#include "../lattice/matrix.h"
#include "../lattice/poly.h"
#include "../common/prng.h"
#include "share.h"

/* ---------- Modular Arithmetic ---------- */

static uint32_t mod_q(int64_t x)
{
    x %= Q;
    if (x < 0)
        x += Q;
    return (uint32_t)x;
}

static uint32_t modinv(uint32_t a)
{
    int64_t t = 0, newt = 1;
    int64_t r = Q, newr = a;

    while (newr != 0) {
        int64_t q = r / newr;
        int64_t tmp;

        tmp = newt;
        newt = t - q * newt;
        t = tmp;

        tmp = newr;
        newr = r - q * newr;
        r = tmp;
    }

    if (r > 1) return 0; // not invertible

    if (t < 0)
        t += Q;

    return (uint32_t)t;
}

/* ---------- Share Generation ---------- */

void shamir_split(const polyvec_l_t *secret,
                  shamir_share_t shares[],
                  int n,
                  int t,
                  qs_prng_t *prng)
{
    uint32_t coeffs[t];

    for (int i = 0; i < n; i++) {
        shares[i].id = i + 1;
        memset(&shares[i].value, 0, sizeof(polyvec_l_t));
    }

    for (int vec = 0; vec < QS_L; vec++) {
        for (int k = 0; k < QS_N; k++) {

            coeffs[0] = secret->vec[vec].coeffs[k];

            for (int j = 1; j < t; j++) {
                uint8_t buf[4];
                prng_squeeze(prng, buf, 4);
                uint32_t r =
                    ((uint32_t)buf[0]) |
                    ((uint32_t)buf[1] << 8) |
                    ((uint32_t)buf[2] << 16) |
                    ((uint32_t)buf[3] << 24);
                coeffs[j] = r % Q;
            }

            for (int s = 0; s < n; s++) {
                uint32_t x = shares[s].id;
                uint32_t y = 0;
                uint32_t x_pow = 1;

                for (int j = 0; j < t; j++) {
                    y = mod_q(y + (uint64_t)coeffs[j] * x_pow);
                    x_pow = mod_q((uint64_t)x_pow * x);
                }

                shares[s].value.vec[vec].coeffs[k] = y;
            }
        }
    }
}

/* ---------- Reconstruction ---------- */

void shamir_reconstruct(polyvec_l_t *result,
                        const shamir_share_t shares[],
                        int t)
{
    memset(result, 0, sizeof(polyvec_l_t));

    for (int vec = 0; vec < QS_L; vec++) {
        for (int k = 0; k < QS_N; k++) {

            uint32_t secret = 0;

            for (int j = 0; j < t; j++) {

                uint32_t xj = shares[j].id;
                uint32_t yj = shares[j].value.vec[vec].coeffs[k];

                uint32_t num = 1;
                uint32_t den = 1;

                for (int m = 0; m < t; m++) {
                    if (m == j) continue;

                    uint32_t xm = shares[m].id;

                    num = mod_q((uint64_t)num * (Q - xm));
                    den = mod_q((uint64_t)den * (xj + Q - xm));
                }

                uint32_t inv = modinv(den);
                uint32_t lagrange = mod_q((uint64_t)num * inv);

                secret = mod_q(secret + (uint64_t)yj * lagrange);
            }

            result->vec[vec].coeffs[k] = secret;
        }
    }
}