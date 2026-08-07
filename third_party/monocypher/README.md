# Monocypher (vendored)

Ed25519 signature **verification** for the firmware-update root of trust
(`alloy/ota/signed.hpp`). Vendored — not fetched at build time — so the code that
authenticates firmware is auditable in-tree and pinned.

| | |
|---|---|
| Version | 4.0.2 |
| Source | https://github.com/LoupVaillant/Monocypher (tag `4.0.2`) |
| Files | `monocypher.[ch]`, `optional/monocypher-ed25519.[ch]` (unmodified) |
| Licence | dual BSD-2-Clause / CC-0 — see `LICENCE.md` |

`monocypher-ed25519.c` provides RFC 8032 Ed25519 (SHA-512 variant); Monocypher's
own EdDSA default uses BLAKE2b and is *not* interoperable with standard tooling,
so the host signer and the device verifier both use the RFC 8032 path.

Only `crypto_ed25519_check` (verify) is reachable from alloy; signing lives on
the host (`alloy keygen` / `alloy image --sign`). Updating: replace the four
files from a release tarball, keep this file's version row honest, and re-run
the signed-image tests — they check real RFC 8032 vectors, not just round-trips.
