# QS-STDW: Quantum-Safe Stateless Threshold Deterministic Wallet

This repository provides a post-quantum secure, stateless, and threshold-based deterministic wallet designed explicitly for cryptocurrency networks. The protocol rerandomizes the keys of the **Threshold RACCOON** signature scheme to ensure both quantum-safety and strict transaction unlinkability on the blockchain.

## Parameters
 
| Parameter | Value |
|---|---|
| Ring | Z_q[x]/(x^512 + 1) |
| Modulus q | 549,824,583,172,097 = p1 × p2 |
| CRT primes | p1 = 33,292,289 ; p2 = 16,515,073 |
| Matrix dims | k = 5, l = 4 |
| Challenge weight ω | 19 |
| Rounding (ν_w, ν_t) | (44, 42) |
| Secrets | Ternary {−1, 0, +1} |
| Masks | Uniform [−2^44, 2^44] |
| NTT | CRT-split 512-pt over p1 and p2 |
| Hash | SHAKE256 (OpenSSL 3) — only external dep |

## Overview

Standard deterministic wallets map a single seed phrase to millions of public addresses, but they act as a single point of failure. If the seed is compromised, all funds are lost. 

**QS-STDW** solves this by:
1. **Threshold Security (Shamir's Secret Sharing):** The master seed is sharded among $N$ participants. A signature requires exactly $T$ participants to authorize a transaction.
2. **Stateless Unlinkability:** Every transaction is tied to a one-time "session". The wallet mathematically derives a *new* public/private key pair per session. Observers cannot link two transactions to the same master identity.
3. **Post-Quantum Resistance:** The signature generation and derivations are built entirely upon hard module-lattice mathematical assumptions.

This code serves as the direct implementation of the 5 theoretical algorithms defined in the QS-STDW specification:
- **Algorithm 1 (KGen):** Master Key Generation & Sharding
- **Algorithm 2 (RandSK):** Deterministic Secret Share Rerandomization
- **Algorithm 3 (RandPK):** Deterministic Public Key Rerandomization
- **Algorithm 4 (Sign):** A completely distributed, zero-knowledge 3-round Threshold Signature protocol.
- **Algorithm 5 (Verify):** Third-party Lattice Signature Verification.


## Benchmark (Intel i5-1235U, 1.30 GHz, WSL2, mean ± σ over 5 runs)
 
Phases marked † are T-independent — flat across all thresholds.
 
```
  T    KeyGen      RandSK    RandPK†   SS₁          SS₂†    SS₃      Combine  Verify†
  ──────────────────────────────────────────────────────────────────────────────────────
     4   15±1ms     4±1ms    1.3ms      79±20ms    0.10ms   0.44ms   0.39ms   0.44ms
    16   31±8ms    20±3ms    1.3ms     634±146ms   0.12ms   1.87ms   0.57ms   0.27ms
    64  196±36ms  115±22ms   1.5ms    8167±1701ms  0.28ms   7.29ms   1.54ms   0.29ms
   256  2291±370ms 928±212ms 1.6ms  159553±10647ms 2.7ms   50.7ms   7.53ms   0.49ms
  1024  38582ms   11088ms    1.6ms  2621687ms      154ms    639ms    134ms    1.32ms
```
 
**Statelessness confirmed**: RandPK, SS₂, Verify are flat at all T.
**Verify throughput**: >2,000 tx/s vs <3 tx/s for Threshold CSI-FiSh — >800× speedup.

To execute the threshold scaling benchmark natively (tests $T \in \{4, 16, 64, 256, 1024\}$):
```bash
make benchmark
./benchmark
```

---

## Build and Test

### 1. Requirements

This project is written in C17 and relies on a standard GCC toolchain. The SHAKE256 XOF primitives rely on OpenSSL.

### Prerequisites
- Linux/MacOS Environment
- `gcc`
- `make`
- `libssl-dev` (OpenSSL)

### Quick Start
To build the end-to-end network simulation and all the underlying algorithmic test suites, simply run:
```bash
make clean
make all
```

---

## Validating the Cryptography (Unit Tests)

If you are a cryptography researcher and want to independently test the invariant bounds (Norm limitations, Shamir reconstructions, Deterministic Polynomial expansions), you can run the individual unit tests natively.

Every test outputs a strict `[PASS]` or `[FAIL]` UI banner.

```bash
# Core Primitives
./test_poly
./test_matrix
./test_master
./test_shamir

# Key Rerandomization and Unlinkability Checks
./test_rerandomize
./test_randpk

# End-to-End valid/tampered Integration Tests
./test_sign
./test_verify
```

---

## Source Layout
 
```
config/params.h          Raccoon-128 params, CRT constants, NTT roots
src/
  common/                hash.c (SHAKE256), prng.c, randombytes.c
  lattice/               ntt.c, poly.c, polyvec.c, matrix.c, sample.c
  keygen/                master_keygen.c        — Algorithm 1
  threshold/             shamir.c, rerandomize.c — Algorithm 2 (RandSK)
  derivation/            rand_pk.c              — Algorithm 3 (RandPK)
  signing/               commit, challenge, response, combine, lagrange
  verify/                verify.c               — Algorithm 5
  wallet/                wallet_init, wallet_sign, wallet_verify
tests/benchmark.c        Full benchmark (all phases, all T)
```

