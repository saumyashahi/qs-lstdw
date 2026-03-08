#ifndef QS_CHALLENGE_POLY_H
#define QS_CHALLENGE_POLY_H

#include <stdint.h>
#include "../lattice/poly.h"

/*
 * Expand a 32-byte Fiat-Shamir hash into a challenge polynomial c ∈ R_q.
 *
 * Following Dilithium/RACCOON convention:
 *   - Exactly TAU nonzero coefficients
 *   - Each nonzero coefficient is ±1
 *   - Remaining N-TAU coefficients are 0
 *
 * This ensures ||c||_1 = TAU and ||c||_∞ = 1.
 *
 * Input : challenge_hash[32]  — output of Hc(pk, m, w)
 * Output: c_poly              — challenge polynomial in R_q
 */
void qs_expand_challenge(poly_t *c_poly, const uint8_t challenge_hash[32]);

#endif /* QS_CHALLENGE_POLY_H */
