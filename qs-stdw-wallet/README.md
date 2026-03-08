# QS-STDW: Quantum-Safe Stateless Threshold Deterministic Wallet

Welcome to the C reference implementation for the **QS-STDW** protocol! 

This repository provides a post-quantum secure, stateless, and threshold-based deterministic wallet designed explicitly for cryptocurrency networks. The protocol rerandomizes the keys of the **Threshold RACCOON** signature scheme to ensure both quantum-safety and strict transaction unlinkability on the blockchain.

---

## 📖 Overview

Standard deterministic wallets map a single seed phrase to millions of public addresses, but they act as a single point of failure. If the seed is compromised, all funds are lost. 

**QS-STDW** solves this by:
1. **Threshold Security (Shamir's Secret Sharing):** The master seed is sharded among $N$ participants. A signature requires exactly $T$ participants to authorize a transaction.
2. **Stateless Unlinkability:** Every transaction is tied to a one-time "session". The wallet mathematically derives a *new* public/private key pair per session. Observers cannot link two transactions to the same master identity.
3. **Post-Quantum Resistance:** The signature generation and derivations are built entirely upon hard module-lattice mathematical assumptions.

### The Cryptographic Papers
This code serves as the direct implementation of the 5 theoretical algorithms defined in the QS-STDW specification:
- **Algorithm 1 (KGen):** Master Key Generation & Sharding
- **Algorithm 2 (RandSK):** Deterministic Secret Share Rerandomization
- **Algorithm 3 (RandPK):** Deterministic Public Key Rerandomization
- **Algorithm 4 (Sign):** A completely distributed, zero-knowledge 3-round Threshold Signature protocol.
- **Algorithm 5 (Verify):** Third-party Lattice Signature Verification.

*For deep, algorithmic implementation details and mapping, please read the [docs/implementation_report.md](docs/implementation_report.md).*

---

## 🛠️ Building the Project

This project is written in C11 and relies on a standard GCC toolchain. The SHAKE256 XOF primitives rely on OpenSSL.

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

## 🌐 Running the P2P Network Simulation

The easiest way to understand the repository is to run the **Network Simulator**. 

The `local_sim` binary spins up a local 5-node network broker, securely shards the master key, and orchestrates a complete 3-of-5 threshold transaction signing over a simulated wire protocol.

```bash
./local_sim
```

**What you will see:**
1. 5 Wallet instances are initialized.
2. A payload (`TRANSFER 10.0 TO 0xXYZ`) is triggered.
3. The Broker selects 3 active signers.
4. The Broker routes the `Commitments`, computes the `Challenge Hash`, and routes the `Response Shares`.
5. The Broker mathematically combines the zero-knowledge shares and outputs the final Signature.
6. A **4th** non-signing validator node mathematically confirms the signature is valid!

---

## 🧪 Validating the Cryptography (Unit Tests)

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

## 📂 Repository Structure

- `src/lattice/` - Core polynomial and matrix bounded mathematical structures ($Z_q$).
- `src/keygen/` - Master key initializations (Algorithm 1).
- `src/threshold/` - Shamir splitting and Secret Rerandomizations (Algorithm 2).
- `src/derivation/` - Public Trapdoor Rerandomizations (Algorithm 3).
- `src/signing/` - The isolated components of the threshold signature (Algorithm 4).
- `src/verify/` - The core mathematical verification constraints (Algorithm 5).
- `src/wallet/` - High-level structural wrappers for nodes, mimicking memory-isolated peers.
- `src/net/` - The P2P network payloads and the broker simulator.
- `docs/` - Academic implementation reports outlining the engineering mappings.
