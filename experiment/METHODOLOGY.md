# FTIE Experiments — Methodology

This document describes every experiment in `experiment/`, exactly as implemented, as source material
for the paper's methodology and results sections. It also serves as the pre-flight check for running
all of them on EC2.

| Experiment | Question | Status | Results (after the run) |
| --- | --- | --- | --- |
| a, b | Cost and quality vs ACM iteration count $n$ | **Not yet run** (re-run planned) | `experiment/results/a.csv`, `b.csv` |
| c, d | Cost and quality vs ACM matrix entry $a$ | **Not yet run** (re-run planned) | `c.csv`, `d.csv` |
| e | Cost and quality vs BBS modulus size | **Not yet run** (re-run planned) | `e.csv` |
| f (corpus) | Quality and cost on real files of 7 types | **Not yet run** | `f.csv` |
| f (size sweep, retired) | Cost and quality vs plaintext size, random data | Retired; not re-run | `f_size_sweep.csv` (pilot) |

**Pilot run.** All of a–f were run once on a VPS on 2026-08-30 (the "pilot"). Its CSVs are in the repo
(commit `c144a47`) and its numbers appear in this document only as **expected values** for the re-run.
They are not the reported results: the pilot used random plaintext for f, its keys and environment
were not recorded, and it ran the code before the fixes in §10. The re-run overwrites
`a.csv`–`e.csv`; the pilot values remain in git history.

**Timing:** one sample per configuration (`TIMING_REPEATS = 1` in `experiment/tools.h`; decided,
§10 F11). Everything else in §10 is fixed in code.

Where the code differs from the requirements in `new-experiment.md`, this document describes the code
and flags the difference. §10 lists the review findings and how each was resolved; §12 is the EC2 run
procedure.

---

## 1. Overview

### 1.1 Purpose

The experiments evaluate FTIE (File-To-Image Encryption) on two axes:

- **Statistical quality of the cipherimage**: does it look like uniform noise, and do two
  encryptions differ as much as two independent random images would?
- **Cost**: encryption and decryption time.

Every experiment runs **two pipelines** on the same input: the **current** pipeline (BBS keystream,
RT, PNG packing, pixel-level ACM) and the **deprecated** pipeline (literal random keystream, RT,
byte-level ACM, then PNG packing). The deprecated pipeline is the baseline the current design is
compared against.

### 1.2 How the experiments fit together

| | a–e | f corpus | f size sweep (retired) |
| --- | --- | --- | --- |
| Independent variable | one FTIE parameter ($n$, $a$, or BBS modulus) | file type and natural file size | plaintext size, 0.5–10 MB |
| Plaintext | 3.21 MB random bytes, same for every row | 692 real files from NapierOne | random bytes, fresh per row |
| Unit of a CSV row | one parameter value | one file | one size |
| Rows | 30 per experiment | one per corpus file | 20 |
| Metrics | NPCR (both variants), UACI, entropy, χ², avalanche, timing | the same, plus plaintext baseline and per-file critical values | pilot only: NPCR (any-channel), entropy, file-byte avalanche, timing |

a–e answer "what does changing a parameter cost?". f (corpus) answers "how does FTIE behave on the
kinds of files it is meant for?". f (corpus) replaces the old size sweep (decision 7 / O8 in
`new-experiment.md`): the old sweep encrypted random bytes, so its quality metrics were measured on
input that was already uniform, and its timing axis is now covered by the corpus files' natural sizes.

---

## 2. System under test (all experiments)

Both pipelines share the same padding and the same RT step. They differ in the keystream source and in
what ACM permutes.

### 2.1 Notation

| Symbol | Meaning |
| --- | --- |
| $P$ | plainfile bytes, length $L$ |
| $P'$ | padded plaintext, length $L' = 3N^2/2$ |
| $K$ | keystream bytes |
| $r_i$ | fresh random byte per plaintext byte (from `std::random_device`, truncated to 8 bits) |
| $C$ | RT output bytes, length $2L' = 3N^2$ |
| $N$ | side of the square cipherimage (always even); $W = H = N$ |
| $a, b, n$ | ACM parameters (matrix entries and iteration count) |
| $p, q, s$ | BBS primes and seed |

### 2.2 Padding (`ftie.cpp: bytes_sequence_padding`)

$N$ is the smallest **even** integer with $N \ge \lceil \sqrt{\lceil 2(L+4)/3 \rceil} \rceil$.
The padded length is $L' = 3N^2/2$. The plaintext is extended with $d = L' - L - 4$ zero bytes,
followed by $d$ as a 4-byte big-endian integer. Decryption reads the last 4 bytes and strips
$d + 4$ bytes.

**Every one of the $3N^2$ cipherimage bytes is RT output.** There are no unencrypted padding pixels;
the padding (zeros and length) is encrypted along with the file (requirement M5). For a 13.9 KB file
the padding is about 3% of $P'$. The 3.21 MB plaintext of a–e gives $N = 1464$; a 10,000,000-byte
file gives $N = 2582$.

### 2.3 Keystream

- **Current pipeline: Blum Blum Shub** (`bbs.cpp`). $m = pq$, $x_0 = s$,
  $x_i = x_{i-1}^2 \bmod m$, $K_i = x_i \bmod 256$ (low 8 bits of each state), for $i = 1..L'$.
  Before generating, the code checks that $p, q$ are prime (GMP Miller–Rabin, 40 rounds),
  $p \equiv q \equiv 3 \pmod 4$, $1 < s < m$ and $\gcd(s, m) = 1$.
- **Deprecated pipeline: literal keystream.** Random bytes from `std::random_device`, indexed as
  $K_{i \bmod |K|}$. $|K|$ = 10,000,000 bytes in a–d, the plaintext size (3,210,000) in e, and in f
  max(10,000,000, largest corpus file + 65,536) so it never wraps within a file (§7.2).

BBS parameters come from `experiment/bbs_params.csv`, produced by `experiment/gen_params.cpp`
(`make experiment-gen-params`): 30 rows of $(bits, p, q, s)$ with prime size stepped evenly from 16 to
1024 bits. All experiments except e use the last row (1024-bit primes, modulus ≈ 2048 bits). The file
is generated once, shared by all experiments and committed with the results (§8).

### 2.4 Randomized Text, RT (`rt.cpp`)

For each padded plaintext byte $P'_i$, with a fresh $r_i$:

$$C_{2i} = K_i + 2P'_i + r_i \pmod{256}, \qquad C_{2i+1} = 2K_i + P'_i + r_i \pmod{256}$$

Decryption: $(C_{2i} - K_i) - (C_{2i+1} - 2K_i) \equiv P'_i$. RT doubles the byte count and makes
encryption **non-deterministic**: the same file encrypted twice under the same key gives different
cipherimages, which differ only through $r$.

### 2.5 Packing and Arnold's Cat Map

**Current pipeline** (`ftie.cpp`, `acm.cpp`): $C$ is packed row-major into an $N \times N$ RGB image
(pixel $j$ = $(C_{3j}, C_{3j+1}, C_{3j+2})$, $x = j \bmod N$, $y = \lfloor j/N \rfloor$), then the
**pixels** are permuted with

$$A = \begin{pmatrix} 1 & a \\ b & 1+ab \end{pmatrix}, \qquad M = A^n \bmod N, \qquad
\text{cipher}(x, y) = \text{plain}\big(M (x, y)^\top\big)$$

computed by modular matrix exponentiation (`mapping_general`). When $a = b$, a closed form is used
instead (`mapping_equal`): $A = B^2$ with $B = \begin{pmatrix} 0 & 1 \\ 1 & a \end{pmatrix}$, so
$A^n = \begin{pmatrix} F_{2n-1} & F_{2n} \\ F_{2n} & F_{2n+1} \end{pmatrix}$ with the generalized
Fibonacci numbers $F_k = aF_{k-1} + F_{k-2} \pmod N$. $M$ is applied once, so the cost barely depends
on $n$. All products use 32-bit unsigned arithmetic, so the matrix is exactly the one above for every
16-bit $a, b$ (§10 F3, F14, F15).

**Deprecated pipeline** (`acm.cpp: ftie::deprecated::acm`): the cat map is applied to the **byte
array** $C$ (length $T = 3N^2$) laid on a $G \times G$ grid, $G = \lfloor\sqrt{T}\rfloor + 1$, keeping
only targets $< T$. The base map is iterated $n$ times, so the cost grows linearly with $n$. The
permuted bytes are then packed into the $N \times N$ image.

Both pipelines give cipherimages of the same size for the same input. Output is an 8-bit RGB PNG
written by png++/libpng with default settings (zlib compression, adaptive row filters).

### 2.6 Build and platform

All binaries are built by the Makefile with `g++ -std=c++17` and **no optimisation flag (`-O0`)**
(`EXTRA_FLAGS` adds include/library paths, e.g. on macOS). All experiments will run on one EC2
instance, Ubuntu 24.04, sharing one `bbs_params.csv`. Each run script appends the machine and code
state to `experiment/results/env.txt` (§8).

---

## 3. Common measurement procedure

Every experiment measures each configuration (a parameter value in a–e, a file in f) with the same
routine, `tools::measure_deprecated` / `tools::measure_current` in `experiment/tools.cpp`, once per
pipeline. Before it, $P_2$ is written as $P_1$ with **bit 1 of byte 0 flipped** (`^= 0x02`; in a–e
$P_1$'s byte 0 is `0x01`, so $P_2$'s is `0x03`). Then:

1. Repeat `TIMING_REPEATS` times: time $\text{Enc}(P_1) \to C_1$; time $\text{Dec}(C_1) \to R$;
   **check $R = P_1$ byte for byte** (a mismatch aborts the configuration). Report the median times.
2. $\text{Enc}(P_1) \to C_2$: a second, independent encryption of the same input.
3. NPCR (any-channel), NPCR (per-channel) and UACI between $C_1$ and $C_2$.
4. Entropy and χ² of $C_1$.
5. $\text{Enc}(P_2) \to C_2$; avalanche between $C_1$ and $C_2$.

All encryptions within one configuration and pipeline use the same key.

### 3.1 Differences between experiments

| | a–e | f (corpus) |
| --- | --- | --- |
| Input check | none (plaintext generated in-process) | file size and SHA-256 checked against the manifest first |
| Round-trip mismatch | experiment aborts (exit 1); `run_experiments.sh` logs it as failed | file recorded as FAILED, no row written, retried next run |
| Extra outputs | — | plaintext entropy and χ², per-file critical values |
| Output | binary prints header and rows to stdout; `run_experiments.sh` writes `<letter>.csv` | `f.cpp` appends to `f.csv` itself; resumable |
| Key | BBS row from `bbs_params.csv`; $a, b, n$ written in every row | frozen in `f.csv.params` (+ `f.csv.keystream`) |

### 3.2 Pairs used by each metric (requirement M4)

| Metric | Pair | What differs between the two cipherimages |
| --- | --- | --- |
| NPCR, NPCR_ch, UACI | $\text{Enc}(P_1)$, $\text{Enc}(P_1)$ | only the RT random bytes $r$ |
| Avalanche | $\text{Enc}(P_1)$, $\text{Enc}(P_2)$ | $r$, plus one plaintext bit |

This NPCR/UACI pair is **not** the classic one-pixel-plaintext-change pair of Wu et al.; it measures
how different two encryptions of identical input are. State this in the paper.

---

## 4. Metrics (`experiment/tools.cpp`)

All cipherimage metrics are computed on the decoded RGB pixels of an $N \times N$ image.
$F = 3WH = 3N^2$ is the number of channel samples. All counters are 64-bit, and every result CSV is
written with 10 significant digits (`tools::CSV_PRECISION`), enough to resolve entropy shortfalls of
$10^{-5}$.

| Metric | Section | a–e | f (corpus) |
| --- | --- | --- | --- |
| Entropy | 4.1 | ✓ | ✓ (cipherimage and plaintext) |
| χ² | 4.2 | ✓ | ✓ (cipherimage and plaintext) |
| NPCR, any-channel | 4.3 | ✓ | ✓ |
| NPCR, per-channel | 4.4 | ✓ | ✓ |
| UACI | 4.5 | ✓ | ✓ |
| Avalanche | 4.6 | ✓ | ✓ |
| Timing | 4.7 | ✓ | ✓ |
| Critical values | 4.8 | (computable from $N = 1464$) | ✓ (columns) |

### 4.1 Entropy (`calculate_entropy`, `calculate_entropy_bytes`)

Shannon entropy of the 256-bin histogram of the R, G and B bytes **pooled**:

$$H = -\sum_{k=0}^{255} p_k \log_2 p_k, \qquad p_k = O_k / F$$

Empty bins contribute 0. Ideal: 8 bits. The plaintext baseline (`entropy_plain`, f only) uses the same
formula over the file's $L$ raw bytes.

### 4.2 Chi-square (`calculate_chi_square`, `calculate_chi_square_bytes`)

Pearson χ² over the same pooled histogram, expected count $E = F/256$ per bin, 255 degrees of freedom:

$$\chi^2 = \sum_{k=0}^{255} \frac{(O_k - E)^2}{E}$$

Critical value 293.25 at α = 0.05 (310.46 at α = 0.01). Valid only when $E \ge 5$, i.e. at least 1,280
bytes; the code does **not** enforce this, so it must be applied in analysis. `chi2_plain` uses the
file's raw bytes. Entropy and χ² come from the same histogram and are approximately linked by
$\chi^2 \approx 2F\ln 2\,(8 - H)$, so they are not independent evidence.

### 4.3 NPCR, any-channel (`calculate_npcr`; column `npcr_*`)

$$\text{NPCR} = \frac{1}{N^2}\sum_{x,y} \mathbb{1}\big[C_1(x,y) \ne C_2(x,y)\big]$$

A pixel counts as different if any channel differs. Fraction in $[0,1]$. Kept for continuity with the
pilot. It is not the literature definition, and its ideal is not 99.6094%: for this design it is
$1 - 2^{-16}$ (current) and ≈ 1 (deprecated) (§10 F7).

### 4.4 NPCR, per-channel (`calculate_npcr_channel`; column `npcr_ch_*`)

$$\text{NPCR}_{ch} = \frac{1}{3N^2}\sum_{c \in \{R,G,B\}}\sum_{x,y} \mathbb{1}\big[C_1^c(x,y) \ne C_2^c(x,y)\big]$$

Mean of the three channel NPCRs. Fraction in $[0,1]$. Ideal for independent uniform images:
$255/256 = 0.996094$. This is the value to compare with the literature.

### 4.5 UACI (`calculate_uaci`)

$$\text{UACI} = \frac{100\%}{3N^2}\sum_{c}\sum_{x,y}\frac{|C_1^c(x,y) - C_2^c(x,y)|}{255}$$

**Percent** in $[0,100]$, unlike the other metrics, which are fractions. Ideal: $257/768 = 33.4635\%$.

### 4.6 Avalanche (`calculate_avalanche`)

Fraction of differing bits between the **decoded pixel bytes** of the two cipherimages
($24N^2$ bits). Ideal 0.5. (The pilot compared the PNG file bytes instead, which reads ≈ 0.4993
because the PNG/zlib framing is identical in both files; §10 F8.)

### 4.7 Timing (`chrono_encrypt`, `chrono_decrypt`, `chrono_deprecated_*`)

Wall-clock time (`std::chrono::high_resolution_clock`) around one full call of encrypt or decrypt,
median over `TIMING_REPEATS` runs. It includes reading the input file, BBS parameter validation (two
40-round Miller–Rabin tests; current pipeline only), keystream generation, RT, ACM, PNG encode/decode
with zlib, and writing the output.

### 4.8 Critical values (requirement M3)

Computed from each cipherimage's own $W, H$, so the threshold tightens as files grow. RT makes the two
bytes of a pair change together between two encryptions, so the $3WH$ channel bytes are only
$1.5WH$ independent samples; the NPCR and UACI thresholds use that count (§10 F2):

| Column | Formula | Test |
| --- | --- | --- |
| `npcr_crit` | $\mu - 1.645\sqrt{\mu(1-\mu)/(1.5WH)}$, $\mu = 255/256$ | fail if `npcr_ch` < value |
| `uaci_crit_low`, `uaci_crit_high` | $100\left(\tfrac{257}{768} \mp 1.96\sqrt{\sigma^2_{pair}/(1.5WH)}\right)$, $\sigma^2_{pair} = 0.0391966$ | fail if `uaci` outside range |
| `entropy_crit` | $8 - 293.25 / (2F\ln 2)$ | fail if entropy < value |
| (constant) | 293.25 | fail if χ² > value |

$\sigma^2_{pair}$ is the exact variance of one RT pair's UACI term $(|\Delta C_{2i}| + |\Delta
C_{2i+1}|)/(2 \cdot 255)$, obtained by enumerating $c_1, c_1', d \in [0,256)^3$ with
$c_2 = c_1 + d$, $c_2' = c_1' + d$ and $d = K - P'$ uniform. The same enumeration gives the
single-byte variance 0.0559930 (Wu et al.: 0.05599) and a within-pair correlation of 0.400. Checked
on the real current pipeline (2,000 re-encryptions of a 20 KB structured file, $N = 116$): per-file
reject rates 5.2% (NPCR_ch) and 4.8% (UACI) at nominal 5%, against 12.9% and 10.0% with the
independent-byte formulas.

Reference values:

| Plainfile | $N$ | `npcr_crit` | UACI range | `entropy_crit` |
| --- | --- | --- | --- | --- |
| 13.9 KB | 98 | 99.524% | 33.14–33.79% | 7.99266 |
| ~1 MB | 818 | 99.599% | 33.42–33.50% | 7.99989 |
| 10 MB | 2582 | 99.606% | 33.45–33.48% | 7.99999 |

---

## 5. Experiments a–e: parameter sweeps

Source: `experiment/a.cpp`–`e.cpp`, run by `run_experiments.sh`. Each sweeps one parameter over
30 values and holds the rest fixed.

### 5.1 Setup

| Item | Value |
| --- | --- |
| Plaintext | 3,210,000 random bytes from `std::random_device`, generated once per experiment and reused for every row. $P_1$ byte 0 = `0x01`, $P_2$ byte 0 = `0x03`. Cipherimage $N = 1464$ |
| Deprecated keystream | 10,000,000 random bytes (e: 3,210,000) |
| BBS | last row of `bbs_params.csv` (1024-bit primes), except e |
| Random $a, b$ | drawn from 1–65535 (ACM rejects 0) |
| Output columns | `prime_bits, acm_a, acm_b, acm_n`, then per pipeline (`_dep`, `_cur`): `npcr, npcr_ch, uaci, entropy, chi2, avalanche, enc_s, dec_s`; e adds `modulus` |

Every row carries its full key: `prime_bits` identifies the `bbs_params.csv` row holding $p, q, s$, and
`acm_a, acm_b, acm_n` are the ACM parameters used. The swept parameter is one of these columns.

### 5.2 What each experiment varies

| Exp. | Swept (30 rows) | Held fixed | Current-pipeline ACM path |
| --- | --- | --- | --- |
| a | $n$ = 10, 20, …, 300 (`acm_n`) | $a = b$, random | `mapping_equal` (closed form) |
| b | $n$ = 10, 20, …, 300 (`acm_n`) | $a \ne b$, random | `mapping_general` |
| c | $a = b$ = 1, 2185, …, 63337, step 2184 (`acm_a`) | $n = 50$ | `mapping_equal` |
| d | $a$ = 1, 2185, …, 63337; $b = a + 1$ (`acm_a`) | $n = 50$ | `mapping_general` |
| e | BBS row: primes 16 → 1024 bits (`prime_bits`, `modulus`) | $a, b$ random, $n = 50$ | `mapping_general` (unless $a = b$) |

The deprecated pipeline never uses BBS, so in e it acts as a control.

### 5.3 What they can and cannot show

**The quality metrics cannot respond to the swept parameter, by construction.**

- **ACM parameters (a–d).** Entropy and χ² come from a histogram, which a permutation does not change.
  NPCR, UACI and pixel-level avalanche compare two cipherimages permuted by the *same* map, so their
  sums do not change either. Changing $a$, $b$ or $n$ changes which permutation is applied, and none
  of these metrics can see that.
- **Key size (e).** RT's fresh $r_i$ makes every cipher byte uniform whatever $K$ is, so a weak
  32-bit modulus and a 2048-bit one give the same histogram and differences.

The pilot agrees. Across all 150 pilot rows (current pipeline), any-channel NPCR stayed at
0.999977–0.999990 and entropy at 7.99997–7.99998, noise around the values RT determines (§10 F7).
Expect the same in the re-run; avalanche will now be ≈ 0.5000 rather than the pilot's ≈ 0.4993 (F8).

**Timing is the informative output.** It does not depend on plaintext content (BBS, RT and ACM cost
depend only on length), so the random plaintext does not weaken it. Pilot values, as expectations for
the re-run (the pilot's c and d timings reflect the bugs fixed in F14 and F15):

| Exp. | Deprecated encrypt (s) | Current encrypt (s) | Reading |
| --- | --- | --- | --- |
| a | 3.7 → 15.9 | 8.9 – 10.0 | deprecated cost grows linearly with $n$ (map iterated $n$ times); current computes $A^n$ once |
| b | 3.7 → 15.9 | 8.2 – 8.5 | same, general path |
| c | 4.5 – 5.4 | 9.1 – 9.2 | flat in $a$ |
| d | 4.5 – 5.5 | 8.2 – 8.3 | flat in $a$ |
| e | 5.35 – 5.38 | 4.0 → 8.3 | current cost grows with modulus size; deprecated (no BBS) flat |

### 5.4 Problems found in the pilot code (fixed for the re-run)

- **c did not actually sweep $a$ in the current pipeline (F14).** A Fibonacci cache keyed only on the
  index reused the first row's $a = b = 1$ values for all 30 rows. Fixed: the closed form now computes
  its Fibonacci numbers fresh per map.
- **d's last 8 rows ($a \ge 48{,}049$) used a different matrix (F15).** `1 + a*b` overflowed `int`.
  Fixed with 32-bit unsigned arithmetic.
- **The random $a, b$ of a, b and e were not recorded (F16).** Fixed: every row carries its key.
- **No round-trip check (F17)**, and decryption overwrote $P_1$. Fixed: decryption goes to a separate
  file and is compared with $P_1$.

Rows within an experiment still reuse one plaintext and one key: 30 measurements of one configuration
each, not independent samples.

### 5.5 How to use a–e in the paper

- Report them as **execution-cost results**: time vs ACM iterations (a, b), vs ACM matrix entry
  (c, d), and vs BBS modulus size (e), with the deprecated pipeline as comparison and control.
- Do **not** use their quality columns as evidence that a parameter affects security. At most they
  show that the metrics stay at their RT-determined values whatever the parameter, as §5.3 predicts.
- State the plaintext (3.21 MB random), the single-sample timing and the build (`-O0`).

---

## 6. Size sweep (old experiment f, retired)

Source: `experiment/f.cpp` as of commit `c7ca275` (replaced in `5556ad2`). It is **not part of the
re-run**; only its pilot results exist, kept as `experiment/results/f_size_sweep.csv`.

### 6.1 Setup

| Item | Value |
| --- | --- |
| Swept | plaintext size $L$ = 500,000 to 10,000,000 bytes in steps of 500,000 (20 rows; column `size_mb` = 0.5–10.0, decimal MB) |
| Plaintext | random bytes from `std::random_device`, **fresh for every row**; byte 0 = `0x01` ($P_1$) or `0x03` ($P_2$) |
| BBS | last row of `bbs_params.csv` (1024-bit primes) |
| ACM | $a, b$ random 16-bit (not recorded), $n = 50$ |
| Deprecated keystream | 10,000,000 random bytes |
| Procedure | the pilot version of §3: no round-trip check, file-byte avalanche, single timing |

### 6.2 Results

Quality metrics are flat across sizes: current-pipeline NPCR 0.999975–0.999991, entropy
7.99981–7.99999 (lowest at the smallest size, as expected for a finite sample), file-byte avalanche
0.49903–0.49937. Timing grows linearly with size; least-squares slopes:

| Operation | Deprecated (s/MB) | Current (s/MB) | Range over 0.5–10 MB (s) |
| --- | --- | --- | --- |
| Encrypt | 1.80 | 2.62 | dep 0.72–17.5, cur 1.29–26.1 |
| Decrypt | 0.92 | 1.61 | dep 0.25–9.1, cur 0.81–16.1 |

### 6.3 Status in the paper

Superseded by f (corpus) for both quality and timing. Its quality numbers are on random plaintext and
should not be used for randomness claims. Its pilot timing comes from a different machine than the
re-run, has no recorded key or environment, and ran the pre-fix code. To report a controlled size
sweep, bring the old `f.cpp` back under a new letter on the shared routine and re-run it with the rest;
otherwise leave it out of the paper.

---

## 7. Experiment f: corpus

Source: `experiment/f.cpp`, `experiment/manifest.{h,cpp}`, `experiment/build_manifest.sh`,
`run_corpus_experiment.sh`.

### 7.1 Dataset

**Source.** NapierOne (Davies, Macfarlane and Buchanan, *Forensic Science International: Digital
Investigation* 40 (2022) 301330, doi:10.1016/j.fsidi.2021.301330). Cite the published version, not the
arXiv preprint, and keep the licence/attribution terms the paper requires (its §3.2, §5.2).

**Subsets.**

| Original file type (FTIE Table I) | NapierOne subset |
| --- | --- |
| DOCX | DOCX |
| XLSX | XLSX |
| PPTX | PPTX |
| PDF | PDF |
| MP3 | MP3 |
| MP4 | MP4-FROM-WEB |
| JPG | JPG-FROM-WEB |
| ZIP | not used |

**Selection.**

- **Tier:** the *Tiny* archive of each subset (sequence numbers 0001–0100). There is no random draw:
  the file list is the archive's fixed list, so a reader can obtain the same files.
- **Size cap:** files up to 10,000,000 bytes at their natural size (`CAP` in `build_manifest.sh`; set
  `CAP=10485760` for 10 MiB, which adds one PDF). Larger files are future work.
- **Excluded:** the subset documentation PDF inside each archive (e.g. `DOCX.pdf`); PSEUDO, PURE,
  `-PASSWORD`, `-NOMAGIC`, EXTRA (ransomware) and ENCRYPTED subsets; JPG QUALITY-\* subsets.
- **Independence:** one subset per type, so no sequence number is drawn from two related subsets.

**Expected sample** (from the S3 listing of the Tiny archives; sizes only, not yet verified by hash):

| Subset | Files ≤ 10,000,000 B | Files ≤ 10 MiB | Median KB | Total MB |
| --- | --- | --- | --- | --- |
| DOCX | 100 | 100 | 45 | 37.8 |
| XLSX | 100 | 100 | 30 | 39.1 |
| PPTX | 94 | 94 | 2,178 | 258.7 |
| PDF | 98 | 99 | 297 | 81.8 |
| MP3 | 100 | 100 | 961 | 86.8 |
| MP4-FROM-WEB | 100 | 100 | 1,449 | 160.2 |
| JPG-FROM-WEB | 100 | 100 | 49 | 8.1 |
| **Total** | **692** | **693** | | **≈ 672** |

The MP4-FROM-WEB Tiny archive does not match the paper's Table 5 for that subset (median 1.45 MB vs a
stated average of 312 KB), and 82 of its 100 file sizes also occur in the MP4 Tiny archive. Report
this as observed; it was checked by size only.

**Manifest.** `experiment/build_manifest.sh <corpus-root> <download-date>` writes
`experiment/corpus_manifest.csv` from the extracted archives (one folder per subset). It keeps files
named `NNNN-…` (dropping the documentation PDFs) up to the cap, sorted by subset and name, with their
SHA-256:

```
type,subset,file_name,path,size_bytes,sha256,archive_tier,download_date
```

The manifest is the provenance record (requirement D7). Fields are unquoted, so no path may contain a
comma (the builder refuses). `f.cpp` re-checks each file's size and SHA-256 against it before
measuring, and uses the SHA-256 as the resume key.

### 7.2 Parameters

| Parameter | Value | Source |
| --- | --- | --- |
| BBS $p, q, s$ | 1024-bit primes $\equiv 3 \pmod 4$; seed coprime to $m$ | last row of `bbs_params.csv` |
| ACM $a, b$ | uniform over 1–65535 from `std::random_device` | drawn by `f.cpp` on first invocation |
| ACM $n$ | 50 | fixed in `f.cpp` |
| Deprecated keystream | max(10,000,000, largest manifest file + 65,536) random bytes | drawn on first invocation |

All of these are drawn once and **frozen**: $p, q, s, a, b, n$, the keystream's length and SHA-256,
`timing_repeats` and the manifest path in `experiment/results/f.csv.params`; the keystream itself in
`f.csv.keystream` (gitignored). A resumed run reloads both and refuses to start if the keystream does
not match its recorded hash.

### 7.3 Procedure

`run_corpus_experiment.sh [manifest] [output]` generates `bbs_params.csv` if missing, builds
`bin/exp-f.o` and runs it. `f.cpp` first refuses to start if the output CSV exists with a different
header (it will not append to old results). Then, for each manifest row in order:

1. **Resume check.** Skip the file if its SHA-256 is already in the output CSV.
2. **Input check.** File size and SHA-256 must match the manifest, else FAILED.
3. **Measure** both pipelines with the §3 routine (deprecated, then current).
4. $W, H$ from the current pipeline's $C_1$; entropy and χ² of $P_1$'s raw bytes; critical values.
5. Append one row to the output CSV and flush.

A FAILED file gets no row and is retried on the next invocation.

---

## 8. Outputs and what to commit

### 8.1 What each experiment records

| Experiment | Results | Key recorded |
| --- | --- | --- |
| a–e | `<letter>.csv`, `<letter>.stderr.log` | every row: `prime_bits` (→ `bbs_params.csv` row), `acm_a, acm_b, acm_n`; e also `modulus` |
| f (corpus) | `f.csv`, `f.stderr.log` | `f.csv.params` (BBS row, $a, b, n$, keystream hash, timing repeats, manifest path, last git commit/compiler/date) |
| all | `env.txt`, `run.log`, `build.log`, `gen-params.log`, `bbs_params.csv` | one `env.txt` block per script invocation: UTC time, git commit and whether code was modified, compiler, EC2 instance type, `uname -a`, `lscpu` |

Not recorded, by design: the RT bytes $r$ and the a–e deprecated keystreams (fresh per process). No
experiment can therefore be reproduced byte for byte; a re-run reproduces the metrics statistically.
The pilot's `bbs_params.csv` was never committed and the pilot VPS is gone, so the pilot keys are lost;
the re-run commits its keys.

### 8.2 Files produced by the EC2 run

| File | Commit? | Notes |
| --- | --- | --- |
| `experiment/bbs_params.csv` | yes | BBS keys of every experiment |
| `experiment/results/a.csv`–`e.csv` | yes | tracked; replace the pilot |
| `experiment/results/a.stderr.log`–`e.stderr.log` | yes | tracked; should be empty |
| `experiment/results/run.log`, `build.log`, `gen-params.log` | yes | tracked; `run.log` **appends** to the pilot's log |
| `experiment/results/f.csv`, `f.csv.params` | yes | |
| `experiment/results/f.stderr.log` | yes | written by `run_corpus_experiment.sh`; check for FAILED lines |
| `experiment/results/env.txt` | yes | written by both run scripts |
| `experiment/corpus_manifest.csv` | yes | provenance; `f.csv.params` records this path |
| `experiment/results/f.archives.sha256` | yes | create by hand: SHA-256 of each downloaded archive |
| `experiment/results/f.csv.keystream` | no (gitignored) | 10 MB; needed only to resume f |
| `experiment/results/<letter>.csv.partial` | no (gitignored) | left only by an interrupted a–e experiment |
| `experiment/dum/*` | no | scratch: 4 files per configuration, ≈ 4 GB for f; keep on S3 if cipherimages are to be re-analysed (current pipeline only) |
| `bin/*` | no | build output |
| NapierOne files | no | size and licence; identified by SHA-256 |

### 8.3 Column reference

**a–e** (one row per sweep value; `_dep` = deprecated, `_cur` = current):
`prime_bits, acm_a, acm_b, acm_n, npcr_dep, npcr_ch_dep, uaci_dep, entropy_dep, chi2_dep,
avalanche_dep, enc_dep_s, dec_dep_s, npcr_cur, npcr_ch_cur, uaci_cur, entropy_cur, chi2_cur,
avalanche_cur, enc_cur_s, dec_cur_s` (+ `modulus` in e).

**f** (one row per file):

| Columns | Meaning |
| --- | --- |
| `type, subset, file_name, sha256, size_bytes` | from the manifest |
| `width, height` | cipherimage size |
| `entropy_plain, chi2_plain` | plaintext baseline |
| `npcr_dep, npcr_cur` | any-channel NPCR (fraction) |
| `npcr_ch_dep, npcr_ch_cur` | per-channel NPCR (fraction) |
| `uaci_dep, uaci_cur` | UACI (percent) |
| `entropy_dep, entropy_cur, chi2_dep, chi2_cur` | cipherimage $C_1$ |
| `avalanche_dep, avalanche_cur` | pixel-level avalanche (fraction) |
| `enc_dep_s, dec_dep_s, enc_cur_s, dec_cur_s` | seconds (median of `TIMING_REPEATS`) |
| `npcr_crit, uaci_crit_low, uaci_crit_high, entropy_crit` | α = 0.05 per-file thresholds (§4.8) |

---

## 9. Analysis plan

No analysis script exists yet (requirement O4).

### 9.1 a–e

1. Plot encrypt and decrypt time against the swept variable for both pipelines; report slopes where
   linear (a, b deprecated vs $n$; e current vs modulus bits). Compare with the pilot (§5.3); a large
   difference points to an environment problem.
2. Report the quality columns only as a constancy check (§5.3). They can also be tested against the
   §4.8 thresholds at $N = 1464$.

### 9.2 f (corpus)

1. **Filter.** Exclude from χ² and entropy any sample under 1,280 bytes (plaintext for `*_plain`, $F$
   for cipherimages). Report excluded files by name.
2. **Per-file tests** at α = 0.05: NPCR_ch (one-sided), UACI (two-sided), entropy (one-sided), χ²
   (one-sided), each against its per-file critical value as written in `f.csv`.
3. **Per-type decision.** Count failures per type and metric. Accept the type if failures do not
   exceed the binomial 99th percentile at $p = 0.05$:

   | Files in type ($n$) | Accept if failures ≤ |
   | --- | --- |
   | 94 (PPTX) | 10 |
   | 98 (PDF, ≤ 10,000,000 B) | 10 |
   | 99 (PDF, ≤ 10 MiB) | 11 |
   | 100 | 11 |

   This keeps each type's false-reject probability under 1% per metric; across 7 types the chance of
   at least one false reject on a metric is about 7%. PPTX and PDF fall slightly below the
   $\min(n\alpha, n(1-\alpha)) \ge 5$ rule of thumb (NIST/SEMATECH e-Handbook §7.2.4); report this.
4. **Descriptive statistics** per type and pipeline: $n$, mean, SD, min, max of every metric. Compare
   entropy across sizes by pass/fail and by shortfall $8 - H$, not raw $H$.
5. **Timing:** time vs `size_bytes` per pipeline with a linear fit (s/MB). The pilot size sweep's
   slopes (§6.2) are a rough expectation only (different machine).
6. **Plaintext vs cipherimage:** contrast `entropy_plain`/`chi2_plain` with the cipherimage values per
   type, to show the output is uniform regardless of input structure.

---

## 10. Findings from the code review and their resolution

Found by reading the code and checking it against a local validation run, the pilot's cipherimages and
targeted tests. All code fixes were verified on 2026-10-16 (macOS, `-O0`); see "Verified" in each
entry.

| ID | Finding | Affected | Status |
| --- | --- | --- | --- |
| F1 | UACI critical range 100× too narrow | f | **fixed** |
| F2 | NPCR_ch/UACI thresholds assumed independent bytes | f | **fixed** |
| F3 | ACM `1 + a*b` overflow | all (random keys), d | **fixed** (with F15) |
| F4 | `bbs_params.csv` missing on a fresh clone | all | **fixed** |
| F5 | Manifest SHA-256 not verified at run time | f | **fixed** |
| F6 | Run-record gaps | all | **fixed** |
| F7 | Any-channel NPCR ideal is not 99.6094% | all | describe in paper |
| F8 | Avalanche measured on PNG file bytes, biased low | all | **fixed** |
| F9 | Flipped bit differed between f and a–e | all | **fixed**: bit 1 everywhere |
| F10 | f's deprecated keystream not frozen; could wrap | f | **fixed** |
| F11 | Timing: single sample, includes I/O | all | **decided**: 1 sample; state in paper |
| F12 | Entropy NaN on empty bins; platform-dependent counters | all | **fixed** |
| F13 | Old `f.csv` collided with the new output | f | **fixed** |
| F14 | c's Fibonacci cache: $a$ not actually swept | c (and any $a = b$ key) | **fixed** |
| F15 | d's top 8 rows used the overflowed matrix | d | **fixed** |
| F16 | a–e did not record their random $a, b$ | a, b, e | **fixed** |
| F17 | a–e had no round-trip check; decryption overwrote $P_1$ | a–e | **fixed** |
| F18 | `run_experiments.sh` skipped half-written CSVs; `--force` changed keys | a–e | **fixed** |
| — | CSVs written with 6 significant digits (entropy unresolvable) | all | **fixed** (found while fixing) |

### 10.1 Statistics

**F1 — UACI critical range units.** The mean was in percent (33.4635) but the half-width in fraction
units, so the range was 100× too narrow (33.4568–33.4702 for a 40×40 image instead of ≈ 32.7–34.3);
almost every file would have failed. *Fix:* half-width in percent (`tools.cpp: uaci_critical_range`).

**F2 — RT byte pairs.** Between two encryptions of the same input, $C_{2i}$ changes if and only if
$r_i$ changes, and so does $C_{2i+1}$, so the $3N^2$ channel bytes are $1.5N^2$ pair samples. With
independent-byte formulas, an ideal cipher fails NPCR_ch in 12.2% of files and UACI in 9.8% at
nominal 5%, and an ideal 100-file type fails the §9.2 rule with probability ≈ 0.57 (NPCR_ch) and
≈ 0.27 (UACI). *Fix:* thresholds with $1.5WH$ samples and the exact pair variance (§4.8).
*Verified:* 2,000 real re-encryptions give 5.2% / 4.8% (old formulas 12.9% / 10.0%).

**F7 — Any-channel NPCR.** Every pixel covers exactly two RT pairs, so in the current pipeline a pixel
is identical between two encryptions only if both $r$ values repeat: ideal $1 - 2^{-16} =
99.99847\%$, matching the pilot (≈ 0.999985). The deprecated pipeline scatters pair bytes across
pixels: ideal $1 - 2^{-24} \approx 1$, matching its 1.0 values. Compare with the literature only via
`npcr_ch`.

**F8 — Avalanche on PNG file bytes.** PNG/zlib framing is identical or near-identical in two
cipherimages of the same size, so file-byte avalanche read ≈ 0.4993 at 3.2 MB (≈ 10 SD below 0.5) and
0.465 for a 561-byte file. On the pilot's `rt_cipherimage_1/2` pair: 0.49936 on file bytes, 0.50003
on decoded pixels (SD under H₀: 0.00007). *Fix:* `calculate_avalanche` compares decoded pixel bytes.
*Verified:* the new function gives 0.50003 on that pair, matching an independent decode.

**F12 — Entropy robustness.** `calculate_entropy` returned NaN when a histogram bin was empty (small
images), and the counters were `uint_fast16_t` (16-bit on macOS, so entropy overflowed for large
images there). *Fix:* empty bins contribute 0; all counters 64-bit. Still exclude samples under 1,280
bytes from χ² and entropy in analysis (§9.2).

**CSV precision.** All CSVs were written with the stream default of 6 significant digits, so entropy
printed as `7.99998` against a critical value of 7.99999. *Fix:* 10 digits (`tools::CSV_PRECISION`).

### 10.2 FTIE code (`ftie/acm.cpp`)

**F3, F15 — `1 + a*b` overflow.** In `A[1][1] = (1 + a * b) % N`, the `uint16_t` operands were
promoted to `int`, so $ab > 2^{31}-1$ was signed overflow. At `-O0` this happened for ≈ 15% of random
keys and gave a wrong matrix; for some sizes (every multiple of 38, and 362, for $a = 60000,
b = 50000$) the map was not even a bijection, so those files could not be decrypted. d's rows
$a \ge 48{,}049$ always hit it. *Fix:* 32-bit unsigned arithmetic in `mapping_general` and in
`modular_matrix_multiplication`.

**F14 — Fibonacci cache.** `mapping_equal` used a `static` cache keyed only on the index, so values
computed for the first $a$ and $N$ in a process were reused for every later one. *Fix:* the
generalized Fibonacci numbers are computed fresh per map, once (not per pixel).

*Verified (F3, F14, F15):* a test including `acm.cpp` compared both mappings against a 64-bit
reference $A^n \bmod N$: 0 mismatches and 0 non-bijective maps over 1,196 random keys (including
$ab > 2^{31}$) across every even $N$ in 4–600; for $a = b$, `mapping_equal` = `mapping_general` =
reference in 897 cases with $a$, $n$, $N$ mixed in one process. Full encrypt/decrypt round trips with
overflowing keys at $N$ = 38, 74 and 362 succeed in both pipelines. Keys with $ab \le 2^{31}-1$ and
$a \ne b$ map exactly as before.

### 10.3 Harnesses and records

**F4 — `bbs_params.csv`.** It was gitignored and the corpus script did not create it, so a fresh
clone crashed in f. *Fix:* both run scripts generate it when missing, `read_bbs_params` fails with a
clear message if it is empty, and the file is committed.

**F5 — SHA-256 at run time.** `f.cpp` now checks each file's size and SHA-256 against the manifest
before measuring. *Verified:* a corrupted hash gives a FAILED line and no row.

**F6 — Run records.** *Fix:* `experiment/record_env.sh`, called by both run scripts, appends a block to
`env.txt` per invocation; `run_corpus_experiment.sh` saves stderr to `f.stderr.log` itself; every a–e
row and `f.csv.params` carry the key; `bbs_params.csv` and the f records are committed normally
(`.gitignore`).

**F9 — Flipped bit.** f flipped bit 0, a–e bit 1. *Fix:* bit 1 everywhere (requirement M4), via
`tools::write_bit1_flipped`.

**F10 — f's deprecated keystream.** It was regenerated at every process start (a resume changed the
deprecated key) and, at 10,000,000 bytes, wrapped for files near the cap. *Fix:* sized past the
largest manifest file, saved to `f.csv.keystream`, its hash recorded in `f.csv.params`, reloaded and
checked on resume. *Verified:* a tampered keystream file stops the run.

**F13 — Old `f.csv`.** The pilot's size-sweep `f.csv` was renamed to `f_size_sweep.csv`, and `f.cpp`
refuses to run against a CSV with a different header, before writing anything.

**F16, F17 — a–e key and round trip.** *Fix:* the shared routine (§3) checks every decryption against
$P_1$ (now never overwritten), and every row carries `prime_bits, acm_a, acm_b, acm_n`. Errors go to
stderr with exit 1 instead of into the CSV.

**F18 — `run_experiments.sh`.** It skipped any non-empty CSV, including a half-written one or the
pilot's, and `--force` regenerated the keys. *Fix:* each experiment writes `<letter>.csv.partial`,
renamed only on exit 0; the old `<letter>.csv` is deleted when the experiment starts; keys are
generated only when missing (`--new-params` to regenerate). *Verified:* an interrupted `--force` run
leaves only the `.partial`, and a plain re-run redoes that experiment with unchanged keys.

### 10.4 Decided

**F11 — Timing design.** Decided: **one sample per configuration** (`TIMING_REPEATS = 1` in
`experiment/tools.h`; the routine supports more and reports the median). It includes file I/O,
PNG/zlib and, for the current pipeline, the fixed cost of BBS parameter validation, which dominates
for small files. The deprecated library functions copy the keystream twice per call (`ftie.cpp`,
`rt.cpp`; part of the implementation, so it stays in the timing). `high_resolution_clock` is the
system (not steady) clock on libstdc++. State in the paper that each timing is a single wall-clock
measurement and what it covers.

---

## 11. Limitations (all experiments)

- **What NPCR/UACI/avalanche measure here.** Because RT adds fresh randomness per byte, the
  differences between cipherimages come from $r$, not from plaintext diffusion. Flipping one plaintext
  bit deterministically changes only the two RT bytes of that position; ACM moves them but does not
  spread the change. So avalanche here measures randomized re-encryption, not the classical
  avalanche/diffusion property. Describe it that way.
- **Pair structure is outside the metric set.** For every plaintext byte,
  $C_{2i} - C_{2i+1} \equiv P'_i - K_i \pmod{256}$, independent of $r$. Histogram tests (entropy,
  χ²) and NPCR/UACI cannot see this, and the same $K$ is used for every input within an experiment.
  A reviewer may ask about byte-pair or correlation tests; state the scope of the metric set.
- **Randomness source.** Cipherimage uniformity rests on `std::random_device` (on Linux/libstdc++,
  the hardware RNG or `/dev/urandom`). The platform is recorded in `env.txt`.
- **Random plaintext in a–e.** Their quality metrics say nothing about behaviour on structured input;
  f (corpus) covers that.
- **Pilot not reported.** The pilot's keys and environment were lost and it ran the pre-fix code; its
  numbers serve only as expectations and should not be mixed with the re-run.
- **Representativeness of f.** It is unknown whether Tiny's 0001–0100 is a representative sample of
  each subset; the NapierOne paper does not say how files were ordered.
- **Previous FTIE results.** The original eight test files are unpublished, so earlier FTIE figures
  cannot be reproduced; f's recorded NapierOne list fixes this for the new results only.

---

## 12. EC2 run procedure (all experiments)

**Before launching (on your machine):** commit and push all code changes, so `env.txt` records a
clean commit (`git_dirty=no`).

**Instance:** Ubuntu 24.04, x86-64 or Graviton, **not burstable** (avoid t3/t4g: CPU credits distort
timing; c7i.large is enough), 20 GB disk. The code is single-threaded. Run nothing else on it, and
run a–e and f one after the other, not in parallel, so they do not disturb each other's timing.

Both run scripts generate `bbs_params.csv` only if it is missing, so a–e and f share one key file in
either order; a–e first is the natural order. Both scripts can simply be re-run after an interruption.

```bash
# 1. Setup
git clone git@github.com:satriahrh/ftie-cpp-dep.git && cd ftie-cpp-dep
./setup_vps.sh
tmux new -s ftie                                              # detach Ctrl-b d, reattach: tmux attach -t ftie

# 2. a–e (≈ 2.4 h): --force replaces the pilot CSVs
./run_experiments.sh --force
#    if interrupted: ./run_experiments.sh   (redoes unfinished letters, same keys)
grep -E "finished|FAILED" experiment/results/run.log | tail -5   # all five "exit 0"
wc -l experiment/results/[a-e].csv                              # 31 lines each (header + 30)

# 3. f corpus: download the 7 Tiny archives and extract one folder per subset:
#    ~/napierone/{DOCX,XLSX,PPTX,PDF,MP3,MP4-FROM-WEB,JPG-FROM-WEB}/
sha256sum ~/napierone-archives/*.zip > experiment/results/f.archives.sha256
experiment/build_manifest.sh ~/napierone <download-date YYYY-MM-DD>
#    expect 100 per subset, PPTX 94, PDF 98 (§7.1)

# 4. f smoke test on 3 files
head -4 experiment/corpus_manifest.csv > experiment/dum/smoke_manifest.csv
./run_corpus_experiment.sh experiment/dum/smoke_manifest.csv experiment/dum/smoke_f.csv

# 5. f full run (≈ 3.2 h); if interrupted, run the same command again
./run_corpus_experiment.sh
wc -l < experiment/results/f.csv; wc -l < experiment/corpus_manifest.csv   # equal
grep -c FAILED experiment/results/f.stderr.log                             # 0

# 6. Commit
git add experiment/bbs_params.csv experiment/corpus_manifest.csv experiment/results/
git status --short                                                       # review before committing
git commit -m "experiment re-run: a-e and corpus f" && git push
```

Also check that no NaN appears in the CSVs and that `env.txt` shows `git_dirty=no`. To push from EC2
the instance needs a GitHub key with write access; otherwise copy the files listed in §8.2 back with
`scp`.

---

## 13. Resource estimate

- **a–e time.** The pilot took 2,210 / 2,132 / 1,610 / 1,517 / 1,198 s for a–e: **≈ 2.4 h** in total.
  The fixed code adds three cheap metrics and a round-trip comparison per configuration.
- **f time.** The pilot size sweep's full procedure (both pipelines, all metrics) ran at about 16.9 s
  per MB of plaintext (1,771 s for 105 MB); for ≈ 672 MB that is about **3.2 h**. A sizing run of one
  10,000,000-byte file through `f.cpp` (macOS/Apple Silicon, `-O0`) took **134 s** (≈ 13.4 s/MB):
  enc/dec 15.4/8.2 s deprecated, 19.2/17.0 s current. The smoke test gives the EC2 rate.
- **Total** ≈ **5.6 h** (`TIMING_REPEATS = 1`).
- **Memory.** Peak resident memory for the 10 MB file was **432 MB**. 2 GB is enough; 4 GB leaves
  margin.
- **Disk.** f's four scratch files per corpus file total about 6× its size (cipherimages are ≈ 2.03×
  the plaintext), so ≈ **4 GB**; a–e add ≈ 100 MB; plus the downloaded archives and extracted files.

---

## 14. Requirements traceability (`new-experiment.md`, f corpus)

| Req. | Status in code |
| --- | --- |
| D1 Loader | Done (`manifest.cpp`, `f.cpp`) |
| D2–D6 Coverage, exclusions, tier, selection, independence | Enforced by `build_manifest.sh` and the archive choice |
| D7 Provenance | Manifest fields; size and SHA-256 re-checked at run time (F5) |
| D8 Attribution | Paper-side |
| M1 UACI | Done, in all experiments |
| M2 χ² | Done, pooled R+G+B, in all experiments |
| M3 Per-image thresholds | Done, with RT-pair sample size (F1, F2) |
| M4 Pair definition | Done; bit 1 of byte 0 everywhere (F9) |
| M5 Padding in sample | Answered: all $3N^2$ bytes are RT output (§2.2) |
| M6 Existing metrics | Kept; avalanche now on decoded pixels (F8) |
| M7 Plaintext baseline | Done |
| O1–O3 Row per file, columns, both pipelines | Done |
| O4 Per-type summary | Not implemented (analysis script) |
| O5 Run record | Done (F6) |
| O6 Resumable | Done, by SHA-256, with frozen keystream (F10) |
| O7 No overwrite of old results | Old size sweep kept as `f_size_sweep.csv`; `f.cpp` refuses a CSV with another header (F13) |
| O8 Timing from corpus | Done |

---

## 15. References

- Davies, S. R., Macfarlane, R., Buchanan, W. J. "NapierOne: A modern mixed file data set alternative
  to Govdocs1." *Forensic Science International: Digital Investigation* 40 (2022) 301330.
  doi:10.1016/j.fsidi.2021.301330
- Wu, Y., Noonan, J. P., Agaian, S. "NPCR and UACI Randomness Tests for Image Encryption."
  *Cyber Journals: JSAT* (2011). Source of $\mu_N = F/(F+1)$, $\mu_U = (F+2)/(3F+3)$ and their
  single-byte variances; this document adapts them to RT's byte pairs (§4.8).
- Wu, Y., Noonan, J. P., Agaian, S. "Shannon Entropy based Randomness Measurement and Test for Image
  Encryption." arXiv:1103.5520.
- NIST/SEMATECH e-Handbook of Statistical Methods, §7.2.4 (sample size for proportions).
- Blum, L., Blum, M., Shub, M. "A Simple Unpredictable Pseudo-Random Number Generator." *SIAM J.
  Computing* 15(2) (1986) 364–383.
