#ifndef QS_STDW_PARAMS_H
#define QS_STDW_PARAMS_H

/*************************************************
 * Global QS-STDW Parameters (Demo Configuration)
 *************************************************/

/* ===============================
 * Ring / Lattice Parameters
 * =============================== */

// Polynomial degree (must be power of 2)
#define QS_N 256

// Modulus (Dilithium prime)
#define Q 8380417

// Dimension parameters (Dilithium-like)
#define QS_K 4      // rows of A
#define QS_L 4      // columns of A

/* ===============================
 * Threshold Parameters
 * =============================== */

// Total number of parties
#define N_PARTIES 3

// Threshold (minimum signers)
#define T_THRESHOLD 2

/* ===============================
 * Rerandomization
 * =============================== */

// Degree of rerandomization polynomial
#define RERAND_DEGREE (T_THRESHOLD - 1)

/* ===============================
 * Sampling Parameters
 * =============================== */

// Bound for small secrets (demo-friendly)
#define ETA 2

// Ephemeral randomness bound
#define GAMMA1 (1 << 17)
#define GAMMA2 ((Q - 1) / 32)

/* ===============================
 * Norm Bounds (Verification)
 * =============================== */

// ||z||_∞ bound
#define BETA_Z  (GAMMA1 - ETA)

// ||w||_∞ bound
#define BETA_W  (GAMMA2)

/* ===============================
 * Hash / Seed Sizes
 * =============================== */

#define SEED_BYTES 32      // 256-bit seeds
#define HASH_BYTES 32
#define SESSION_ID_BYTES 32

/* ===============================
 * Rounding Parameters
 * =============================== */

// Used in round_ν functions
#define NU_W 8
#define NU_T 8

#endif /* QS_STDW_PARAMS_H */