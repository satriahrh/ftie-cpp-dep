# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Branch note

The former `experiment` branch (`origin/experiment`) has been merged into `master` (the "Merge
remote-tracking branch 'origin/experiment'" commit) — `master` is the current line of work. That merge
added:

- A `Makefile` with `experiment-build-*` / `experiment-run-*` targets.
- An `experiment/` directory (`a.cpp`..`f.cpp`, `tools.h`/`tools.cpp`) containing standalone comparison
  harnesses that benchmark the current image-based pipeline against the deprecated byte-array pipeline
  (NPCR, entropy, avalanche effect, timing).
- `ftie::deprecated::encrypt`/`decrypt` (in `ftie.h`/`ftie.cpp`) and `ftie::deprecated::acm::encrypt`/`decrypt`
  (in `acm.h`/`acm.cpp`): a byte-array-based ACM predecessor to the current PNG-based ACM, kept for comparison.
- `main.cpp` accepts an alternate 8-argument invocation that dispatches to the deprecated pipeline (see below).

## Build

Requires `g++` with C++17, `libpng` (`libpng-config` must be on `PATH`), `png++` (a header-only C++
wrapper over libpng — install via `brew install png++` on macOS, or the `png++` package on Linux; it is
included as `"png++/png.hpp"` so it must be resolvable via the compiler's include path), and `gmp`/`gmpxx`
(arbitrary-precision arithmetic for the BBS modulus — `brew install gmp` on macOS, or `libgmp-dev` on Linux).

Build the CLI (from repo root):
```
g++ -o bin/ftie-cli -std=c++17 ftie/*.h ftie/*.cpp main.cpp `libpng-config --ldflags` -lgmpxx -lgmp
```
On macOS/Homebrew (confirmed on Apple Clang 17), the above fails two ways this project's g++/Linux
assumption doesn't hit: Clang errors on `ftie/*.h` being compiled as separate translation units, and
`/opt/homebrew` isn't on the default search path the way `libpng-config` handles libpng. Use instead:
```
g++ -o bin/ftie-cli -std=c++17 -I/opt/homebrew/include -L/opt/homebrew/lib ftie/*.cpp main.cpp `libpng-config --cflags --ldflags` -lgmpxx -lgmp
```
(same `-I`/`-L` and dropped `ftie/*.h` applies to the Makefile's `experiment-build-*` targets too, if
building those on macOS.)

Use the Makefile to build/run the comparison harnesses:
```
make experiment-build-a   # ... through experiment-build-f, or experiment-build-all
make experiment-run-a     # ... through experiment-run-f, or experiment-run-all
```
Each experiment binary writes scratch input/output files under `experiment/dum/` (gitignored).

## Running

```
./bin/ftie-cli <encrypt|decrypt> <in-path> <out-path> P Q S A B N
```
- `P`, `Q`, `S`: Blum Blum Shub parameters, passed as decimal-string CLI args and parsed into GMP `mpz_class`
  values — `P`, `Q` are primes congruent to 3 mod 4 (BBS validates this via `ftie::is_prime`, Miller-Rabin at
  40 rounds), sized so `P*Q` is a ~2048-bit modulus (`P`, `Q` each ~1024 bits); `S` is the seed, coprime to
  `P*Q`. Generate a fresh prime with `./bin/ftie-cli genprime <bits>` (prints one decimal prime to stdout;
  run it twice for distinct `P`/`Q`). `encrypt` and `decrypt` must be called with the same `P`/`Q`/`S`.
- `A`, `B`, `N`: Arnold's Cat Map parameters — `A`, `B` are the map's matrix entries, `N` is the iteration count.

An 8-argument deprecated mode is also supported:
```
./bin/ftie-cli <encrypt|decrypt> <in-path> <out-path> <keystream-string> A B N
```
which routes to `ftie::deprecated::encrypt`/`decrypt` instead, using a literal keystream string in place of BBS.

There is no unit test suite. Correctness/quality is instead checked empirically via the `experiment/`
harnesses (NPCR, entropy, avalanche effect) described above.

## Architecture

This implements FTIE (File-To-Image Encryption): an arbitrary file is turned into a stream cipher over
bytes, packed into a square PNG, then the pixels are permuted. Everything lives under the `ftie` namespace,
split across `ftie/` by responsibility:

- **`main.cpp`** — CLI entry point only: parses argv, dispatches to `ftie::encrypt`/`ftie::decrypt`.
- **`ftie/ftie.{h,cpp}`** — orchestrates the full pipeline and owns the file/image byte-conversion helpers
  (`physical_file_to_bytes_sequence`, `bytes_sequence_to_image`, and their inverses). Encrypt order:
  read file → pad bytes (`bytes_sequence_padding`, sized so post-RT byte count maps onto a square RGB
  image) → BBS keystream → RT encrypt (doubles byte count) → pack into square PNG → ACM permute pixels →
  write PNG. Decrypt runs this in reverse, ending with `bytes_sequence_stripping` to remove the padding.
- **`ftie/bbs.{h,cpp}`** — Blum Blum Shub PRNG (`generate_randoms`), operating on GMP `mpz_class` `p`/`q`/`s`
  so the modulus can be ~2048 bits; validates `p`/`q` primality and the `3 mod 4` congruence, and that
  `gcd(s, p*q) == 1`, before generating the keystream.
- **`ftie/prime.{h,cpp}`** — `ftie::is_prime` (Miller-Rabin via `mpz_probab_prime_p`, 40 rounds) and
  `ftie::generate_prime_congruent_3_mod_4(bits)`, used by `bbs` to validate `p`/`q` and exposed via the CLI's
  `genprime` subcommand to mint fresh ones.
- **`ftie/rt.{h,cpp}`** — "Randomized Text": combines keystream + plaintext byte with a fresh
  `std::random_device` value per byte into two output bytes; decrypt recovers the plaintext byte via
  subtraction of the two reconstructed halves. This is what doubles the byte count between plaintext and
  cipherimage.
- **`ftie/acm.{h,cpp}`** — Arnold's Cat Map pixel permutation over a square `png::image`. Uses a closed-form
  Fibonacci-based mapping (`mapping_equal`) when `a == b`, otherwise modular matrix exponentiation
  (`mapping_general`); `get_map` picks between them. On `experiment`, also defines
  `ftie::deprecated::acm::encrypt`/`decrypt`, the predecessor ACM operating directly on `vector<uint8_t>`
  (no image packing), used only by the deprecated pipeline.

When editing the pipeline, keep in mind the byte-count arithmetic in `ftie.cpp`'s padding/stripping and
`bytes_sequence_to_image`/`image_to_bytes_sequence` in sync — the padding size is derived from the same
square-image-sizing formula used when packing bytes into pixels, and RT's 2x byte expansion is baked into
both directions.

## Repo layout

- `data/` — assets referenced by `README.md` (system block diagrams).
- `ftie/` — the library described above.
- `main.cpp` — CLI entry point.
