# FTIE Experiments — Methodology

This document describes every experiment in `experiment/`, exactly as implemented, as source material
for the paper's methodology and results sections. It also serves as the pre-flight check for running
all of them on EC2.

| Experiment | Question | Status | Results |
| --- | --- | --- | --- |
| a, b | Cost and quality vs ACM iteration count $n$ | **Re-run pending** (seed chaining) | `experiment/results/a.csv`, `b.csv` |
| c, d | Cost and quality vs ACM matrix entry $a$ | **Re-run pending** (seed chaining) | `c.csv`, `d.csv` |
| e | Cost and quality vs BBS modulus size | **Re-run pending** (seed chaining) | `e.csv` |
| f (corpus) | Quality and cost on real files of 7 types | **Re-run pending** (seed chaining) | `f.csv` |
| Analysis | Per-type decisions, size-free checks, timing fits (§8) | Updated for seed chaining (`experiment/analyze.py`) | `experiment/results/analysis/` |

**Re-run with seed chaining.** The 2026-10-16 run (clean commit `cb6c1cf`, results `25c9e8a`) measured
the proposed pipeline with the BBS seed $s$ restarted for every encryption, so every encryption under
one key used the same keystream $K$. The paper's algorithm instead passes the last BBS state on as the
next message's seed (§2.3), and its KPA defence depends on that. The code now chains the seed, and a–f
are re-run with the same design, keys, corpus, instance type, compiler and `-O0`. Only the proposed
pipeline's critical values change (§4.8). The 2026-10-16 results in §8.4 are superseded and will be
replaced by the re-run's.

**Timing:** one sample per configuration (`TIMING_REPEATS = 1` in `experiment/tools.h`; §4.7).

**Pipeline names.** The two pipelines are **proposed** (BBS keystream, pixel-level ACM; `ftie::encrypt`,
measured by `tools::measure_proposed`) and **previous** (literal keystream, byte-level ACM;
`ftie::previous::encrypt`, measured by `tools::measure_previous`). Result columns end in `_proposed`
and `_previous`.

Where the code differs from the requirements in `new-experiment.md`, this document describes the code
and flags the difference. §9 lists the implementation checks; §11 is the EC2 run procedure.

---

## 1. Overview

### 1.1 Purpose

The experiments evaluate FTIE (File-To-Image Encryption) on two axes:

- **Statistical quality of the cipherimage**: does it look like uniform noise, and do two
  encryptions differ as much as two independent random images would?
- **Cost**: encryption and decryption time.

Every experiment runs **two pipelines** on the same input: the **proposed** pipeline (BBS keystream,
RT, PNG packing, pixel-level ACM) and the **previous** pipeline (literal random keystream, RT,
byte-level ACM, then PNG packing). The previous pipeline is the baseline the proposed design is
compared against.

### 1.2 How the experiments fit together

| | a–e | f corpus |
| --- | --- | --- |
| Independent variable | one FTIE parameter ($n$, $a$, or BBS modulus) | file type and natural file size |
| Plainfile | 3.21 MB random bytes, same for every row | 692 real files from NapierOne |
| Unit of a CSV row | one parameter value | one file |
| Rows | 30 per experiment | one per corpus file |
| Metrics | NPCR (both variants), UACI, entropy, χ², avalanche, timing | the same, plus plainfile baseline and per-file critical values |

a–e answer "what does changing a parameter cost?". f (corpus) answers "how does FTIE behave on the
kinds of files it is meant for?". f encrypts real files rather than random bytes (decision 7 / O8 in
`new-experiment.md`), so its quality metrics are measured on structured input, and it gives cost
against size through the files' natural sizes.

---

## 2. System under test (all experiments)

Both pipelines share the same padding and the same RT step. They differ in the keystream source and in
what ACM permutes.

### 2.1 Notation

| Symbol | Meaning |
| --- | --- |
| $P$ | plainfile bytes, length $L$ |
| $P'$ | padded plainfile, length $L' = 3N^2/2$ |
| $K$ | keystream bytes |
| $r_i$ | fresh random byte per plainfile byte (from `std::random_device`, truncated to 8 bits) |
| $C$ | RT output bytes, length $2L' = 3N^2$ |
| $N$ | side of the square cipherimage (always even); $W = H = N$ |
| $a, b, n$ | ACM parameters (matrix entries and iteration count) |
| $p, q, s$ | BBS primes and seed |
| $s_j$ | seed of the $j$-th encryption under one key: $s_1 = s$, $s_{j+1} = x_{L'_j}$ (§2.3) |

### 2.2 Padding (`ftie.cpp: bytes_sequence_padding`)

$N$ is the smallest **even** integer with $N \ge \lceil \sqrt{\lceil 2(L+4)/3 \rceil} \rceil$.
The padded length is $L' = 3N^2/2$. The plainfile is extended with $d = L' - L - 4$ zero bytes,
followed by $d$ as a 4-byte big-endian integer. Decryption reads the last 4 bytes and strips
$d + 4$ bytes.

**Every one of the $3N^2$ cipherimage bytes is RT output.** There are no unencrypted padding pixels;
the padding (zeros and length) is encrypted along with the file (requirement M5). For a 13.9 KB file
the padding is about 3% of $P'$. The 3.21 MB plainfile of a–e gives $N = 1464$; a 10,000,000-byte
file gives $N = 2582$.

### 2.3 Keystream

- **Proposed pipeline: Blum Blum Shub** (`bbs.cpp`). $m = pq$, $x_0 = s$,
  $x_i = x_{i-1}^2 \bmod m$, $K_i = x_i \bmod 256$ (low 8 bits of each state), for $i = 1..L'$.
  Before generating, the code checks that $p, q$ are prime (GMP Miller–Rabin, 40 rounds),
  $p \equiv q \equiv 3 \pmod 4$, $1 < s < m$ and $\gcd(s, m) = 1$.
  **Seed chaining.** `bbs::generate_randoms` also returns the last state $x_{L'}$, and
  `ftie::encrypt` returns it as the seed of the next message under the same $p, q$; `ftie::decrypt`
  takes the seed its message was encrypted with. So the $j$-th encryption uses $s_j$ with
  $s_1 = s$ and $s_{j+1} = x_{L'_j}$, and consecutive messages take consecutive, non-overlapping
  stretches of one BBS sequence. The CLI prints the next seed after each encryption. The keystream of
  one message ($x_0 = s_j$, $x_i = x_{i-1}^2 \bmod m$, $K_i = x_i \bmod 256$) is unchanged. The
  smallest modulus in e (16-bit primes) has a BBS cycle of 30.5 M states, against ≈ 9.6 M used per
  configuration, so chaining never wraps the cycle within a row.
- **Previous pipeline: literal keystream.** Random bytes from `std::random_device`, indexed as
  $K_{i \bmod |K|}$. It is fixed for every encryption, as in its original design (no chaining). $|K|$ = 10,000,000 bytes in a–d, the plainfile size (3,210,000) in e, and in f
  max(10,000,000, largest corpus file + 65,536) so it never wraps within a file (§6.2).

BBS parameters come from `experiment/bbs_params.csv`, produced by `experiment/gen_params.cpp`
(`make experiment-gen-params`): 30 rows of $(bits, p, q, s)$ with prime size stepped evenly from 16 to
1024 bits. All experiments except e use the last row (1024-bit primes, modulus ≈ 2048 bits). The file
is generated once, shared by all experiments and committed with the results (§7).

### 2.4 Randomized Text, RT (`rt.cpp`)

For each padded plainfile byte $P'_i$, with a fresh $r_i$:

$$C_{2i} = K_i + 2P'_i + r_i \pmod{256}, \qquad C_{2i+1} = 2K_i + P'_i + r_i \pmod{256}$$

Decryption: $(C_{2i} - K_i) - (C_{2i+1} - 2K_i) \equiv P'_i$. RT doubles the byte count and makes
encryption **non-deterministic**: the same file encrypted twice gives different cipherimages. In the
previous pipeline they differ only through $r$ (same $K$). In the proposed pipeline they differ through
$r$ and $K$, because the second encryption uses the next seed.

The map $(K_i, r_i) \mapsto (C_{2i}, C_{2i+1})$ is invertible mod 256 (determinant $1 \cdot 1 - 2
\cdot 1 = -1$). So for uniform $K_i, r_i$ the two bytes of a pair are independent and uniform. Two
encryptions with independent $K$ and $r$ therefore give four independent uniform bytes per pair
position. With a shared $K$, the two pairs differ by the same $\delta = r' - r$ in both bytes (§4.8).

### 2.5 Packing and Arnold's Cat Map

**Proposed pipeline** (`ftie.cpp`, `acm.cpp`): $C$ is packed row-major into an $N \times N$ RGB image
(pixel $j$ = $(C_{3j}, C_{3j+1}, C_{3j+2})$, $x = j \bmod N$, $y = \lfloor j/N \rfloor$), then the
**pixels** are permuted with

$$A = \begin{pmatrix} 1 & a \\ b & 1+ab \end{pmatrix}, \qquad M = A^n \bmod N, \qquad
\text{cipher}(x, y) = \text{plain}\big(M (x, y)^\top\big)$$

computed by modular matrix exponentiation (`mapping_general`). When $a = b$, a closed form is used
instead (`mapping_equal`): $A = B^2$ with $B = \begin{pmatrix} 0 & 1 \\ 1 & a \end{pmatrix}$, so
$A^n = \begin{pmatrix} F_{2n-1} & F_{2n} \\ F_{2n} & F_{2n+1} \end{pmatrix}$ with the generalized
Fibonacci numbers $F_k = aF_{k-1} + F_{k-2} \pmod N$. $M$ is applied once, so the cost barely depends
on $n$. All products use 32-bit unsigned arithmetic, so the matrix is exactly the one above for every
16-bit $a, b$ (§9).

**Previous pipeline** (`acm.cpp: ftie::previous::acm`): the cat map is applied to the **byte
array** $C$ (length $T = 3N^2$) laid on a $G \times G$ grid, $G = \lfloor\sqrt{T}\rfloor + 1$, keeping
only targets $< T$. The base map is iterated $n$ times, so the cost grows linearly with $n$. The
permuted bytes are then packed into the $N \times N$ image.

Both pipelines give cipherimages of the same size for the same input. Output is an 8-bit RGB PNG
written by png++/libpng with default settings (zlib compression, adaptive row filters).

### 2.6 Build and platform

All binaries are built by the Makefile with `g++ -std=c++17` and **no optimisation flag (`-O0`)**
(`EXTRA_FLAGS` adds include/library paths, e.g. on macOS). All experiments ran on one EC2 c7i.large
(Intel Xeon Platinum 8488C, Ubuntu 24.04, kernel 6.17.0-1017-aws, g++ 13.3.0), sharing one
`bbs_params.csv`, a–e first and then f, with nothing else running. Each run script appended the machine
and code state to `experiment/results/env.txt` (§7).

---

## 3. Common measurement procedure

Every experiment measures each configuration (a parameter value in a–e, a file in f) with the same
routine, `tools::measure_previous` / `tools::measure_proposed` in `experiment/tools.cpp`, once per
pipeline. Before it, $P_2$ is written as $P_1$ with **bit 1 of byte 0 flipped** (`^= 0x02`; in a–e
$P_1$'s byte 0 is `0x01`, so $P_2$'s is `0x03`). Then:

1. Repeat `TIMING_REPEATS` times: time $\text{Enc}(P_1) \to C_1$; time $\text{Dec}(C_1) \to R$;
   **check $R = P_1$ byte for byte** (a mismatch aborts the configuration). Report the median times.
2. $\text{Enc}(P_1) \to C_2$: a second, independent encryption of the same input.
3. NPCR (any-channel), NPCR (per-channel) and UACI between $C_1$ and $C_2$.
4. Entropy and χ² of $C_1$.
5. $\text{Enc}(P_2) \to C_2$; avalanche between $C_1$ and $C_2$.

All encryptions within one configuration and pipeline use the same key: the previous pipeline's
literal keystream, or the proposed pipeline's $p, q$. The proposed pipeline keeps **one running seed
per key** and passes it through every encryption in this fixed order. Step 1's encryption (and its
decryption) uses seed `seed_c1`, step 2's `seed_c2`, step 5's `seed_c3`, and the seed after step 5 is
`seed_next`, the first seed of the next configuration. All four are written to the row. The chain
starts at the key's $s$ in each experiment process: a, b, c, d and f each start from the last
`bbs_params.csv` row's $s$, and in e every row is its own key and starts from its own $s$. With
`TIMING_REPEATS` > 1 each timed repeat takes one chain step, and `seed_c1` is the seed of the $C_1$
that is kept (the run uses 1).

### 3.1 Differences between experiments

| | a–e | f (corpus) |
| --- | --- | --- |
| Input check | none (plainfile generated in-process) | file size and SHA-256 checked against the manifest first |
| Round-trip mismatch | experiment aborts (exit 1); `run_experiments.sh` logs it as failed | file recorded as FAILED, no row written, retried next run |
| Extra outputs | — | plainfile entropy and χ², per-file critical values |
| Output | binary prints header and rows to stdout; `run_experiments.sh` writes `<letter>.csv` | `f.cpp` appends to `f.csv` itself; resumable |
| Key | BBS row from `bbs_params.csv`; $a, b, n$ and the proposed pipeline's seeds written in every row | frozen in `f.csv.params` (+ `f.csv.keystream`); seeds in every row |

### 3.2 Pairs used by each metric (requirement M4)

| Metric | Pair | What differs: previous pipeline | What differs: proposed pipeline |
| --- | --- | --- | --- |
| NPCR, NPCR_ch, UACI | $\text{Enc}(P_1)$, $\text{Enc}(P_1)$ | only the RT random bytes $r$ | $r$ and $K$ (seeds `seed_c1`, `seed_c2`) |
| Avalanche | $\text{Enc}(P_1)$, $\text{Enc}(P_2)$ | $r$, plus one plainfile bit | $r$, $K$ (`seed_c1`, `seed_c3`), plus one plainfile bit |

The two pipelines' pairs therefore have different null distributions (§4.3, §4.6, §4.8).

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
| Entropy | 4.1 | ✓ | ✓ (cipherimage and plainfile) |
| χ² | 4.2 | ✓ | ✓ (cipherimage and plainfile) |
| NPCR, any-channel | 4.3 | ✓ | ✓ |
| NPCR, per-channel | 4.4 | ✓ | ✓ |
| UACI | 4.5 | ✓ | ✓ |
| Avalanche | 4.6 | ✓ | ✓ |
| Timing | 4.7 | ✓ | ✓ |
| Critical values | 4.8 | (computable from $N = 1464$) | ✓ (columns) |

### 4.1 Entropy (`calculate_entropy`, `calculate_entropy_bytes`)

Shannon entropy of the 256-bin histogram of the R, G and B bytes **pooled**:

$$H = -\sum_{k=0}^{255} p_k \log_2 p_k, \qquad p_k = O_k / F$$

Empty bins contribute 0. Ideal: 8 bits. The plainfile baseline (`entropy_plain`, f only) uses the same
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

A pixel counts as different if any channel differs. Fraction in $[0,1]$. It is
not the literature definition, and its ideal is not 99.6094%. Both pipelines now have the ideal
$1 - 2^{-24} \approx 1$: the previous pipeline scatters pair bytes across pixels, and in the proposed
pipeline the two encryptions use different $K$, so the three channel bytes of a pixel are independent
between them (§2.4). (Without seed chaining, as in the 2026-10-16 run, a proposed-pipeline pixel was
identical only if both of its RT pairs' $r$ repeated, with ideal $1 - 2^{-16} = 99.99847\%$, which the
run matched.) Compare with the literature only via `npcr_ch` (§4.4).

### 4.4 NPCR, per-channel (`calculate_npcr_channel`; column `npcr_ch_*`)

$$\text{NPCR}_{ch} = \frac{1}{3N^2}\sum_{c \in \{R,G,B\}}\sum_{x,y} \mathbb{1}\big[C_1^c(x,y) \ne C_2^c(x,y)\big]$$

Mean of the three channel NPCRs. Fraction in $[0,1]$. Ideal for independent uniform images:
$255/256 = 0.996094$. This is the value to compare with the literature.

### 4.5 UACI (`calculate_uaci`)

$$\text{UACI} = \frac{100\%}{3N^2}\sum_{c}\sum_{x,y}\frac{|C_1^c(x,y) - C_2^c(x,y)|}{255}$$

**Percent** in $[0,100]$, unlike the other metrics, which are fractions. Ideal: $257/768 = 33.4635\%$.

### 4.6 Avalanche (`calculate_avalanche`)

Fraction of differing bits between the **decoded pixel bytes** of the two cipherimages
($24N^2$ bits). Ideal 0.5. (Comparing the PNG file bytes instead reads ≈ 0.4993, because the
PNG/zlib framing is identical in both files.)

**Null spread (analysis only, no pass rule).** It depends on the pipeline.

- **Previous pipeline (shared $K$).** Between $\text{Enc}(P_1)$ and $\text{Enc}(P_2)$ both bytes of an
  RT pair shift by the same $\delta = r' - r$, so their bit flips are correlated (the lowest bit flips in
  both or in neither). The $24N^2$ bits are therefore not independent, and $0.5/\sqrt{24N^2}$
  understates the spread. Enumerating $c, c', \delta \in [0,256)^3$ (the same method that gives
  $\sigma^2_{pair}$ in §4.8, which it reproduces exactly) gives a variance of **5.77777 bits²** per
  16-bit pair against 4.0 for independent bits, so
  $$SD_0 = \sqrt{5.77777 / (256 \cdot 1.5 N^2)} \approx 1.20 \times 0.5/\sqrt{24N^2}.$$
  A Monte Carlo of the RT equations (structured plainfile, $N = 64$, 4,000 pairs) rejects 5.25% at
  $|z| > 1.96$ with $SD_0$, against 10.0% with the naive SD.
- **Proposed pipeline (chained seeds).** The two encryptions use different $K$, so all $24N^2$ bits are
  independent (§2.4) and the naive $SD = 0.5/\sqrt{24N^2}$ applies. Checked on real chained
  re-encryptions together with §4.8: in the same 2,000 chained runs, 5.35% of
  pairs have $|z| > 1.96$ with the naive SD ($z$ SD 1.005), against 2.10% with $SD_0$.

The analysis reports $z = (\text{avalanche} - 0.5)/SD$ per file with each pipeline's SD but sets
**no pass/fail rule**, because: (1) here avalanche measures randomized re-encryption, not diffusion
(§10), so a "pass" would invite a diffusion claim the test cannot support; and (2) avalanche was not in
the pre-set rule set (requirement left "Pending"), so a threshold added after seeing the data would be
post hoc.

### 4.7 Timing (`chrono_encrypt`, `chrono_decrypt`, `chrono_previous_*`)

Wall-clock time (`std::chrono::high_resolution_clock`) around one full call of encrypt or decrypt,
median over `TIMING_REPEATS` runs. It includes reading the input file, BBS parameter validation (two
40-round Miller–Rabin tests; proposed pipeline only), keystream generation, RT, ACM, PNG encode/decode
with zlib, and writing the output.

Each configuration is timed **once** (`TIMING_REPEATS = 1` in `experiment/tools.h`; the routine
supports more and reports the median). The proposed pipeline's fixed cost of BBS parameter validation
was about 10 ms in the run (§8.4), so it dominates only for files of a few KB. The previous-pipeline
library functions copy the keystream twice per call (`ftie.cpp`, `rt.cpp`); this is part of the
implementation, so it stays in the timing. `high_resolution_clock` is the system (not steady) clock on
libstdc++. State in the paper that each timing is a single wall-clock measurement and what it covers.

### 4.8 Critical values (requirement M3)

Computed from each cipherimage's own $W, H$, so the threshold tightens as files grow. The NPCR and
UACI nulls depend on whether the two encryptions of a pair share $K$ (§3.2), so each pipeline has its
own critical values (`tools::npcr_critical_value` / `uaci_critical_range`, argument
`sharedKeystream`):

- **Previous pipeline (shared $K$): RT-pair values.** RT makes the two bytes of a pair change together
  between two encryptions, so the $3WH$ channel bytes are only $1.5WH$ independent samples, with the
  pair variance $\sigma^2_{pair}$.
- **Proposed pipeline (chained seeds): independent-byte values** (Wu et al. 2011, $F = 3WH$ samples).
  The two encryptions use different $K$, so all $3WH$ bytes are independent (§2.4), with the
  single-byte variance $\sigma^2_{byte}$.

| Column | Formula | Test |
| --- | --- | --- |
| `npcr_crit_previous` | $\mu - 1.645\sqrt{\mu(1-\mu)/(1.5WH)}$, $\mu = 255/256$ | fail if `npcr_ch_previous` < value |
| `npcr_crit_proposed` | $\mu - 1.645\sqrt{\mu(1-\mu)/(3WH)}$ | fail if `npcr_ch_proposed` < value |
| `uaci_crit_low_previous`, `uaci_crit_high_previous` | $100\left(\tfrac{257}{768} \mp 1.96\sqrt{\sigma^2_{pair}/(1.5WH)}\right)$, $\sigma^2_{pair} = 0.0391966$ | fail if `uaci_previous` outside range |
| `uaci_crit_low_proposed`, `uaci_crit_high_proposed` | $100\left(\tfrac{257}{768} \mp 1.96\sqrt{\sigma^2_{byte}/(3WH)}\right)$, $\sigma^2_{byte} = 0.0559930$ | fail if `uaci_proposed` outside range |
| `entropy_crit` (both) | $8 - 293.25 / (2F\ln 2)$ | fail if entropy < value |
| (constant, both) | 293.25 | fail if χ² > value |

Entropy and χ² are unaffected: they come from one cipherimage's histogram, which is uniform either way.

$\sigma^2_{pair}$ is the exact variance of one RT pair's UACI term $(|\Delta C_{2i}| + |\Delta
C_{2i+1}|)/(2 \cdot 255)$, obtained by enumerating $c_1, c_1', d \in [0,256)^3$ with
$c_2 = c_1 + d$, $c_2' = c_1' + d$ and $d = K - P'$ uniform. The same enumeration gives the
single-byte variance $\sigma^2_{byte}$ = 0.0559930 (Wu et al.: 0.05599) and a within-pair correlation
of 0.400.

**Validation.** Each set was checked on real re-encryptions of a structured file at nominal 5%:

- RT-pair values, with $K$ shared as in the previous pipeline (2026-10-16 check, proposed pipeline
  before seed chaining; 2,000 re-encryptions of a 20 KB structured file, $N = 116$): per-file reject
  rates 5.2% (NPCR_ch) and 4.8% (UACI), against 12.9% and 10.0% with the independent-byte values. The
  previous pipeline uses the same RT code and a fixed $K$, and its byte-level ACM only reorders the
  bytes, which leaves the sums unchanged, so this check applies to it.
- Independent-byte values, with chained seeds (proposed pipeline, 2026-11-02, macOS, `-O0`; 2,000 chained
  re-encryptions of the first 20,000 bytes of this document, $N = 116$, last `bbs_params.csv` row,
  $a = 40{,}820$, $b = 61{,}719$, $n = 50$): per-file reject rates
  5.30% (NPCR_ch) and 5.75% (UACI), both within 1.5 binomial standard errors (0.49 points) of 5%, with
  $z$ SD 1.008 and 1.019. The RT-pair values reject only 1.40% and 1.95% here: with chained seeds they
  are too lenient.

Reference values:

| Plainfile | $N$ | `npcr_crit` previous / proposed | UACI range previous | UACI range proposed | `entropy_crit` |
| --- | --- | --- | --- | --- | --- |
| 13.9 KB | 98 | 99.524% / 99.549% | 33.14–33.79% | 33.19–33.74% | 7.99266 |
| ~1 MB | 818 | 99.599% / 99.602% | 33.42–33.50% | 33.43–33.50% | 7.99989 |
| 3.21 MB (a–e) | 1464 | 99.604% / 99.605% | 33.44–33.49% | 33.45–33.48% | 7.99997 |
| 10 MB | 2582 | 99.606% / 99.607% | 33.45–33.48% | 33.45–33.47% | 7.99999 |

---

## 5. Experiments a–e: parameter sweeps

Source: `experiment/a.cpp`–`e.cpp`, run by `run_experiments.sh`. Each sweeps one parameter over
30 values and holds the rest fixed.

### 5.1 Setup

| Item | Value |
| --- | --- |
| Plainfile | 3,210,000 random bytes from `std::random_device`, generated once per experiment and reused for every row. $P_1$ byte 0 = `0x01`, $P_2$ byte 0 = `0x03`. Cipherimage $N = 1464$ |
| Previous-pipeline keystream | 10,000,000 random bytes (e: 3,210,000), fresh per process |
| BBS | last row of `bbs_params.csv` (1024-bit primes), except e; one running seed per experiment from its $s$ (§3) |
| Fixed $a, b$ (a, b, e) | **pinned** to the values drawn at random in the 2026-10-16 run: a $a = b = 50{,}175$; b $a = 43{,}005$, $b = 51{,}352$; e $a = 48{,}945$, $b = 49{,}252$ |
| Output columns | `prime_bits, acm_a, acm_b, acm_n`, then per pipeline (`_previous`, `_proposed`): `npcr, npcr_ch, uaci, entropy, chi2, avalanche, enc_s, dec_s`, then `seed_c1_proposed, seed_c2_proposed, seed_c3_proposed, seed_next_proposed`; e adds `modulus` |

Every row carries its full key: `prime_bits` identifies the `bbs_params.csv` row holding $p, q, s$,
`acm_a, acm_b, acm_n` are the ACM parameters used, and the seed columns are the proposed pipeline's
seeds (§3). The swept parameter is one of these columns.

The fixed $a, b$ were drawn from 1–65535 in the 2026-10-16 run and are now constants in `a.cpp`,
`b.cpp` and `e.cpp`, so the re-run differs from that run only by seed chaining. The previous pipeline's
random keystream was never recorded, so it is drawn afresh, as before. In a, $a = b = 50{,}175$ gives
the pixel map period $P(1464) = 30$, so for $n$ = 30, 60, …, 300 (10 of 30 rows) the proposed
pipeline's ACM is the identity (§8.1). The metrics cannot see this (§5.3).

### 5.2 What each experiment varies

| Exp. | Swept (30 rows) | Held fixed | Proposed-pipeline ACM path |
| --- | --- | --- | --- |
| a | $n$ = 10, 20, …, 300 (`acm_n`) | $a = b$, pinned | `mapping_equal` (closed form) |
| b | $n$ = 10, 20, …, 300 (`acm_n`) | $a \ne b$, pinned | `mapping_general` |
| c | $a = b$ = 1, 2185, …, 63337, step 2184 (`acm_a`) | $n = 50$ | `mapping_equal` |
| d | $a$ = 1, 2185, …, 63337; $b = a + 1$ (`acm_a`) | $n = 50$ | `mapping_general` |
| e | BBS row: primes 16 → 1024 bits (`prime_bits`, `modulus`) | $a, b$ pinned, $n = 50$ | `mapping_general` |

The previous pipeline never uses BBS, so in e it acts as a control.

### 5.3 What they can and cannot show

**The quality metrics cannot respond to the swept parameter, by construction.**

- **ACM parameters (a–d).** Entropy and χ² come from a histogram, which a permutation does not change.
  NPCR, UACI and pixel-level avalanche compare two cipherimages permuted by the *same* map, so their
  sums do not change either. Changing $a$, $b$ or $n$ changes which permutation is applied, and none
  of these metrics can see that.
- **Key size (e).** RT's fresh $r_i$ makes every cipher byte uniform whatever $K$ is, so a weak
  32-bit modulus and a 2048-bit one give the same histogram and differences.

The run confirms this (§8.4): in all 300 rows the quality metrics stay at their RT-determined
values, at most 3 of 30 rows fail any test, no metric correlates with the swept parameter, and
avalanche is ≈ 0.5000.

**Timing is the informative output.** It does not depend on plainfile content (BBS, RT and ACM cost
depend only on length), so the random plainfile does not weaken it. The timing results are in §8.4
(H8a–H8c).

Rows within an experiment reuse one plainfile and one key (one running seed in a–d): 30 measurements
of one configuration each, not independent samples.

### 5.4 How to use a–e in the paper

- Report them as **execution-cost results**: time vs ACM iterations (a, b), vs ACM matrix entry
  (c, d), and vs BBS modulus size (e), with the previous pipeline as comparison and control.
- Do **not** use their quality columns as evidence that a parameter affects security. At most they
  show that the metrics stay at their RT-determined values whatever the parameter, as §5.3 predicts.
- State the plainfile (3.21 MB random), the single-sample timing and the build (`-O0`).

---

## 6. Experiment f: corpus

Source: `experiment/f.cpp`, `experiment/manifest.{h,cpp}`, `experiment/build_manifest.sh`,
`run_corpus_experiment.sh`.

### 6.1 Dataset

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

**Sample.** The S3 listing of the Tiny archives predicted the counts below. The run matched them: the
manifest built on EC2 (download date 2026-10-16) holds 692 files, each one's size and SHA-256 was
re-checked before it was measured, and the seven archives' own SHA-256 are in
`experiment/results/f.archives.sha256`.

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

### 6.2 Parameters

| Parameter | Value | Source |
| --- | --- | --- |
| BBS $p, q, s$ | 1024-bit primes $\equiv 3 \pmod 4$; seed coprime to $m$ | last row of `bbs_params.csv` |
| ACM $a, b$ | uniform over 1–65535 from `std::random_device` | drawn by `f.cpp` on first invocation |
| ACM $n$ | 50 | fixed in `f.cpp` |
| Previous-pipeline keystream | max(10,000,000, largest manifest file + 65,536) random bytes | drawn on first invocation |
| Proposed-pipeline seed | one running seed, starting at $s$, carried through the files in order | §3; recorded per row |

The key is $a = 40{,}820$, $b = 61{,}719$ (so `mapping_general`), $n = 50$, a 10,000,000-byte
keystream and the 1024-bit row of `bbs_params.csv`. It was drawn on the first invocation of the
2026-10-16 run and **frozen**: $p, q, s, a, b, n$, the keystream's length and SHA-256,
`timing_repeats`, the manifest path and `seed_chain=per-run` in `experiment/results/f.csv.params`; the
keystream itself in `f.csv.keystream` (gitignored). The re-run reuses this key unchanged and writes a
fresh `f.csv`, so it differs from the 2026-10-16 run only by seed chaining. A resumed run reloads both
and refuses to start if the keystream does not match its recorded hash.

**Running seed.** `f.csv.params` holds the chain's start $s$; the running seed lives in `f.csv`. Each
row records `seed_c1_proposed` … `seed_next_proposed`, and the next file starts from the last row's
`seed_next_proposed`. A resumed run reads it from there. It refuses to start if the first row's
`seed_c1_proposed` is not the frozen $s$ (CSV and params from different runs), or if the last row is
incomplete (a run killed while writing it), so a torn seed can never start the next file. A FAILED file
does not advance the chain: the next file starts where the last written row ended, as a resumed run
would. The `seed_*` columns of consecutive rows therefore form one unbroken chain in CSV order, which
need not be manifest order if a file was retried.

### 6.3 Procedure

`run_corpus_experiment.sh [manifest] [output]` generates `bbs_params.csv` if missing, builds
`bin/exp-f.o` and runs it. `f.cpp` first refuses to start if the output CSV exists with a different
header (it will not append to old results). Then, for each manifest row in order:

1. **Resume check.** Skip the file if its SHA-256 is already in the output CSV.
2. **Input check.** File size and SHA-256 must match the manifest, else FAILED.
3. **Measure** both pipelines with the §3 routine (previous, then proposed, from the running seed).
4. $W, H$ from the proposed pipeline's $C_1$; entropy and χ² of $P_1$'s raw bytes; critical values
   for each pipeline (§4.8).
5. Append one row to the output CSV and flush; only then does the running seed advance to the row's
   `seed_next_proposed`.

A FAILED file gets no row and is retried on the next invocation.

---

## 7. Outputs and what to commit

### 7.1 What each experiment records

| Experiment | Results | Key recorded |
| --- | --- | --- |
| a–e | `<letter>.csv`, `<letter>.stderr.log` | every row: `prime_bits` (→ `bbs_params.csv` row), `acm_a, acm_b, acm_n`, the proposed pipeline's four seeds; e also `modulus` |
| f (corpus) | `f.csv`, `f.stderr.log` | `f.csv.params` (BBS row, $a, b, n$, keystream hash, seed chain mode, timing repeats, manifest path, last git commit/compiler/date); every row: the proposed pipeline's four seeds |
| all | `env.txt`, `run.log`, `build.log`, `gen-params.log`, `bbs_params.csv` | one `env.txt` block per script invocation: UTC time, git commit and whether code was modified, compiler, EC2 instance type, `uname -a`, `lscpu` |

Not recorded, by design: the RT bytes $r$ and the a–e previous-pipeline keystreams (fresh per process). No
experiment can therefore be reproduced byte for byte; a re-run reproduces the metrics statistically.
The run's keys are committed (`bbs_params.csv`, `f.csv.params`), and so is the seed of every
proposed-pipeline encryption (the `seed_*` columns), so any recorded encryption's keystream can be
regenerated.

### 7.2 Files produced by the EC2 run

| File | Commit? | Notes |
| --- | --- | --- |
| `experiment/bbs_params.csv` | yes | BBS keys of every experiment |
| `experiment/results/a.csv`–`e.csv` | yes | tracked |
| `experiment/results/a.stderr.log`–`e.stderr.log` | yes | tracked; should be empty |
| `experiment/results/run.log`, `build.log`, `gen-params.log` | yes | tracked; `run.log` **appends** across runs |
| `experiment/results/f.csv`, `f.csv.params` | yes | |
| `experiment/results/f.stderr.log` | yes | written by `run_corpus_experiment.sh`; check for FAILED lines |
| `experiment/results/env.txt` | yes | written by both run scripts |
| `experiment/corpus_manifest.csv` | yes | provenance; `f.csv.params` records this path |
| `experiment/results/f.archives.sha256` | yes | create by hand: SHA-256 of each downloaded archive |
| `experiment/results/f.csv.keystream` | no (gitignored) | 10 MB; needed to resume f and to re-run it with the same key (keep a copy) |
| `experiment/results/<letter>.csv.partial` | no (gitignored) | left only by an interrupted a–e experiment |
| `experiment/dum/*` | no | scratch: 4 files per configuration, ≈ 4 GB for f |
| `bin/*` | no | build output |
| NapierOne files | no | size and licence; identified by SHA-256 |

Produced afterwards, locally, by `experiment/analyze.py` (§8):

| File | Commit? | Notes |
| --- | --- | --- |
| `experiment/results/analysis/report.txt` | yes | everything the script prints: integrity check, tables, fits |
| `experiment/results/analysis/f_decisions.csv` | yes | type × test × pipeline: n, failures, cutoff, accept/reject; pooled rows |
| `experiment/results/analysis/f_uniformity.csv` | yes | KS, Kruskal–Wallis and Spearman results (§8.2 step 3) |
| `experiment/results/analysis/f_summary.csv` | yes | per type and pipeline: n, mean, SD, min, max of every metric (O4) |
| `experiment/results/analysis/f_per_file.csv` | yes | per file: χ² p-value, z-scores, pass/fail, included flag |
| `experiment/results/analysis/excluded.csv` | yes | files excluded by the 1,280-byte rule |
| `experiment/results/analysis/timing_fits.csv`, `ae_quality.csv` | yes | timing fits (f and a–e); a–e quality checks |
| `experiment/results/analysis/*.png` | yes | `f_timing`, `f_entropy`, `f_chi2_pvalues`, `f_plain_vs_cipher`, `f_statistics_by_type`, `ae_timing` |
| `.venv/` | no | Python environment for the script (gitignored) |

### 7.3 Column reference

**a–e** (one row per sweep value):
`prime_bits, acm_a, acm_b, acm_n, npcr_previous, npcr_ch_previous, uaci_previous, entropy_previous,
chi2_previous, avalanche_previous, enc_previous_s, dec_previous_s, npcr_proposed, npcr_ch_proposed,
uaci_proposed, entropy_proposed, chi2_proposed, avalanche_proposed, enc_proposed_s, dec_proposed_s,
seed_c1_proposed, seed_c2_proposed, seed_c3_proposed, seed_next_proposed` (+ `modulus` in e).

**f** (one row per file):

| Columns | Meaning |
| --- | --- |
| `type, subset, file_name, sha256, size_bytes` | from the manifest |
| `width, height` | cipherimage size |
| `entropy_plain, chi2_plain` | plainfile baseline |
| `npcr_previous, npcr_proposed` | any-channel NPCR (fraction) |
| `npcr_ch_previous, npcr_ch_proposed` | per-channel NPCR (fraction) |
| `uaci_previous, uaci_proposed` | UACI (percent) |
| `entropy_previous, entropy_proposed, chi2_previous, chi2_proposed` | cipherimage $C_1$ |
| `avalanche_previous, avalanche_proposed` | pixel-level avalanche (fraction) |
| `enc_previous_s, dec_previous_s, enc_proposed_s, dec_proposed_s` | seconds (median of `TIMING_REPEATS`) |
| `npcr_crit_previous, npcr_crit_proposed` | α = 0.05 per-file NPCR_ch thresholds, per pipeline (§4.8) |
| `uaci_crit_low_previous, uaci_crit_high_previous, uaci_crit_low_proposed, uaci_crit_high_proposed` | α = 0.05 per-file UACI ranges, per pipeline (§4.8) |
| `entropy_crit` | α = 0.05 per-file entropy threshold, both pipelines (§4.8) |
| `seed_c1_proposed, seed_c2_proposed, seed_c3_proposed, seed_next_proposed` | proposed pipeline's BBS seeds, decimal (§3) |

---

## 8. Analysis

`experiment/analyze.py` implements this section (requirement O4). It reads `experiment/results/` and
`experiment/corpus_manifest.csv` and writes `experiment/results/analysis/` (§7.2). Run it from the repo
root:

```
python3 -m venv .venv && .venv/bin/pip install -r experiment/requirements.txt
make experiment-analyze          # = .venv/bin/python experiment/analyze.py
```

It exits non-zero if the integrity check finds a problem. Per-file critical values are read from
`f.csv`, not recomputed; the script only checks that they match the §4.8 formulas.

### 8.1 a–e

1. Plot encrypt and decrypt time against the swept variable for both pipelines (`ae_timing.png`).
   Fit a line per pipeline and operation: slope, $R^2$, p of the slope, and the fitted change over
   the sweep as a percentage of the mean time. For e, also a quadratic fit (§8.4 shows why).
2. Report the quality columns only as a constancy check (§5.3): test each row against its pipeline's
   §4.8 thresholds at $N = 1464$ (z-scores and avalanche SD per pipeline, as in §8.2 step 4), and test
   each metric for a trend in the swept variable (Spearman).
3. **Seed chain** (part of the integrity check): in a–d, the first row's `seed_c1_proposed` is the last
   `bbs_params.csv` row's $s$ and every row's `seed_c1_proposed` is the previous row's
   `seed_next_proposed`; in e, every row's `seed_c1_proposed` is its own key's $s$; in every row the
   four seeds are distinct.
4. **ACM identity flag** (proposed pipeline only): for each row, the period $P(N)$ of the pixel map
   $A = \begin{pmatrix} 1 & a \\ b & 1+ab \end{pmatrix} \bmod N$ at $N = 1464$. Where $P(N)$ divides
   $n$, $A^n \equiv I$ and the proposed pipeline's ACM leaves the image unchanged. The count per
   experiment goes to `ae_quality.csv` (`acm_identity_rows`). The previous pipeline's byte-level map
   (on a $G \times G$ grid, keeping only targets $< T$) is not this map and is not flagged.

### 8.2 f (corpus)

0. **Integrity.** Every `f.csv` row matches a manifest row by SHA-256 (type, subset, name and size
   agree), no duplicates, no missing files; $W = H$, even, and equal to $N$ from the §2.2 formula; the
   critical-value columns match §4.8 for each pipeline; `f.stderr.log` has one `done:` line per row and
   nothing else; the seed chain is unbroken: the first row's `seed_c1_proposed` is `f.csv.params`'s
   $s$, every row's `seed_c1_proposed` is the previous row's `seed_next_proposed`, and the four seeds of
   a row are distinct.
1. **Filter.** Exclude from χ² and entropy any sample under 1,280 bytes (plainfile for `*_plain`,
   $F$ for cipherimages). The cipherimage exclusion also covers NPCR_ch and UACI, whose normal
   approximation needs about the same size (at $F < 1{,}280$ fewer than 3 pairs are expected to be
   unchanged). Excluded files are listed by name (`excluded.csv`).
2. **Per-file tests** at α = 0.05: NPCR_ch (one-sided), UACI (two-sided), entropy (one-sided), χ²
   (one-sided), each against its per-file critical value as written in `f.csv`.
3. **Per-type decision.** Count failures per type and metric. Accept the type if failures do not
   exceed the binomial 99th percentile at $p = 0.05$:

   | Files in type ($n$) | Accept if failures ≤ |
   | --- | --- |
   | 94 (PPTX) | 10 |
   | 98 (PDF, ≤ 10,000,000 B) | 10 |
   | 99 (JPG after the exclusion; PDF ≤ 10 MiB) | 11 |
   | 100 | 11 |

   This keeps each type's false-reject probability under 1% per metric. Across the 56 decisions
   (7 types × 4 tests × 2 pipelines) about 0.56 false rejects are expected. A pooled row per test
   (all files) is tested against Binomial($n$, 0.05). PPTX and PDF fall slightly below the
   $\min(n\alpha, n(1-\alpha)) \ge 5$ rule of thumb (NIST/SEMATECH e-Handbook §7.2.4); report this.
4. **Size-free checks.** Raw entropy, NPCR and UACI depend on file size,
   so each file's statistic is converted to one with a size-free null distribution: the χ² p-value
   (df 255; Uniform(0,1) under H₀), and z-scores for NPCR_ch and UACI (§4.8 μ and σ, with each
   pipeline's own null: $3WH$ independent bytes for proposed, $1.5WH$ pairs for previous) and avalanche
   (naive SD for proposed, $SD_0$ for previous, §4.6), each N(0,1) under H₀. Then, per type and pooled: KS test
   against the null distribution; across types: Kruskal–Wallis (type effect); against
   `entropy_plain`: Spearman (plainfile-structure effect). Entropy gets no separate check here: it
   comes from the same histogram as χ² ($\chi^2 \approx 2F\ln 2\,(8-H)$), so its p-value would repeat
   χ²'s.
5. **Descriptive statistics** per type and pipeline: $n$, mean, SD, min, max of every metric. Compare
   entropy across sizes by pass/fail and by shortfall $8 - H$, not raw $H$ (`f_entropy.png`).
6. **Avalanche:** mean, SD, min, max and the $z$ summary per type; no pass/fail (§4.6).
7. **Any-channel NPCR model check:** mean against $1 - 2^{-24}$ for both pipelines (§4.3).
8. **Timing:** time vs `size_bytes` per pipeline with a linear fit (s/MB, intercept, $R^2$). Flag a
   file as an outlier only if its residual exceeds both 4 robust SD (1.4826 × MAD) and 10% of the
   fitted time; the proposed pipeline's fit is so tight that 4 robust SD alone is a few hundredths of
   a second.
9. **Plainfile vs cipherimage:** contrast `entropy_plain`/`chi2_plain` with the cipherimage values per
   type, to show the output is uniform regardless of input structure. The report gives the per-type
   means and the number of files passing χ² ≤ 293.25 for the plainfile and each pipeline.
   `f_plain_vs_cipher.png` is a per-file strip plot by type, plainfile next to both cipherimages:
   χ² on a log scale with the critical value, and entropy with the 8-bit ideal. Entropy has no single
   threshold there, because its critical value depends on size; per-file entropy pass/fail is in
   `f_entropy.png`. The plainfile and cipherimage χ² cover different sample counts ($L$ against
   $F \approx 2L$). For non-uniform data χ² grows with the count, so this difference favours the
   plainfile. Compare the two by pass/fail, not by ratio.
10. **ACM identity flag** (proposed pipeline only, as in §8.1 step 4): $P(N)$ at each file's own $N$
    for the frozen $a, b$, and whether it divides $n = 50$. `f_per_file.csv` gets `acm_period` and
    `acm_identity`, and the report names the flagged files. With $a = 40{,}820$, $b = 61{,}719$ that is
    three files ($P = 10$ at $N = 110$ and $N = 290$). The metrics cannot see the permutation (§5.3), so
    the flag explains the files rather than excluding them.

### 8.3 Hypotheses

| # | Hypothesis | Evidence |
| --- | --- | --- |
| H1 | **Uniformity.** The cipherimage byte histogram is uniform for every file type | χ², entropy (§8.2 steps 2–4) |
| H2 | **Re-encryption independence.** Two encryptions of the same file differ like two independent uniform images | NPCR_ch, UACI (steps 2–4) |
| H3 | **Avalanche ≈ 0.5** (as randomized re-encryption, §10) | avalanche, descriptive + $z$ (step 6) |
| H4 | **Input independence.** Output quality does not depend on file type or plainfile entropy | Kruskal–Wallis, Spearman, step 9 |
| H5 | **Pipeline parity.** Each pipeline is as consistent with its own null as the other on H1–H4. The pipelines are tested against different nulls for NPCR_ch, UACI and avalanche (proposed: independent bytes, since chained seeds give each encryption its own $K$; previous: RT pairs sharing $K$, §4.8), so parity means similar failure rates and size-free statistics (p-values, z-scores) consistent with their nulls, not equal raw NPCR_ch/UACI values against one threshold. χ² and entropy share one null | the same tables, both pipelines |
| H6 | **Cost is linear in size**; proposed has a larger slope and fixed cost | step 8 |
| H7 | a–e quality metrics do not change with $n$, $a$, $b$ or modulus size (§5.3) | §8.1 step 2 |
| H8a | Previous encrypt time grows linearly with $n$; proposed is flat (a, b) | §8.1 step 1 |
| H8b | Time is flat in the matrix entry $a$ (c, d) | §8.1 step 1 |
| H8c | Proposed time grows with modulus size; previous is flat (e) | §8.1 step 1 |

### 8.4 Results (2026-10-16 run, superseded)

**Superseded.** These results come from the code before seed chaining, in which every
proposed-pipeline encryption under one key used the same $K$. They were analysed with the RT-pair null
for both pipelines and a $1 - 2^{-16}$ any-channel ideal for proposed. The seed-chained re-run replaces
them, and §8.5's figures with them. Kept here until the re-run's results are in.

All figures below are from `experiment/results/analysis/report.txt` of that run.

**Integrity.** No issues: 692/692 rows match the manifest, every side matches the padding formula,
the critical-value columns match §4.8, and the log has only `done:` lines. One file is excluded,
JPG `0074-jpg.jpg` (302 B, cipherimage 768 B), from every test; 691 files remain.

**Corpus f.**

| # | Result |
| --- | --- |
| H1 | Supported. All 7 types accept χ² and entropy in both pipelines. Pooled failures: χ² 34/691 (proposed), 35/691 (previous), about 35 expected; binomial p = 1.0 and 0.93. χ² and entropy fail on exactly the same files. χ² p-values are uniform (pooled KS p = 0.85 proposed, 0.26 previous). Cipherimage χ² ranges 180–326 (proposed) and 193–346 (previous). |
| H2 | Supported. All types accept. NPCR_ch failures 38/691 (proposed) and 33/691 (previous); UACI 33/691 and 28/691; all pooled binomial p ≥ 0.29. Means: NPCR_ch 0.996088 / 0.996109 (ideal 0.996094); UACI 33.465% / 33.459% (ideal 33.4635%). |
| H3 | Mean avalanche 0.500015 (proposed), 0.49998 (previous). $z$ mean 0.04 / −0.01, SD 1.00 / 0.94; files with $\lvert z\rvert > 1.96$: 34 and 27 of 691. |
| H4 | Supported. **No plainfile passes χ² (0 of 691; minimum 685), against 657 (proposed) and 656 (previous) of 691 cipherimages** (`f_plain_vs_cipher.png`). Plainfile entropy averages 7.66 (XLSX) to 7.95 (MP4) per type and plainfile χ² 3.3×10⁴ (JPG) to 2.3×10⁶ (PDF), while cipherimage χ² averages 252–257 in every type. No type effect (Kruskal–Wallis p ≥ 0.46) except previous-pipeline UACI (p = 0.039); no plainfile-entropy effect (Spearman $\lvert\rho\rvert$ ≤ 0.053, p ≥ 0.16). |
| H5 | Supported. The pipelines are indistinguishable on every quality metric. |
| H6 | Supported, with a small fixed cost. Proposed: encrypt 2.599 s/MB, decrypt 1.592 s/MB, $R^2$ ≥ 0.99996, no outliers. Previous: 1.753 / 0.930 s/MB, $R^2$ 0.997 / 0.968, 4 / 31 outliers. The proposed pipeline's fixed cost (BBS validation, I/O) is about 10 ms: 0.0118 s against 0.0034 s for the 302-byte file. |
| — | Any-channel NPCR: 0.99998515 (proposed, ideal 0.99998474) and 0.99999984 (previous, ideal 0.99999994), as §4.3 predicts. |

Multiplicity: 0 of 56 type decisions reject (about 0.56 expected). Of the 80 size-free tests, 5 give
p < 0.05 (about 4 expected); all 5 involve UACI (proposed pooled KS p = 0.044, proposed MP4 0.044,
previous MP4 0.042, previous JPG 0.008, previous Kruskal–Wallis 0.039). The previous pipeline's
avalanche $z$ SD of 0.94 is 2.3 standard errors below 1 (χ² variance test p ≈ 0.02); both pipelines
use the same RT code, so this is read as chance among 16 such comparisons.

**a–e.**

| # | Result |
| --- | --- |
| H7 | Supported. At most 3 of 30 rows fail any test (1.5 expected), every metric stays at its RT-determined value (NPCR_ch 0.99609, UACI 33.46%, entropy 7.99997, avalanche 0.5000), and no metric trends with the parameter (one Spearman p < 0.05 of 40: proposed UACI in a, p = 0.033). One χ² KS p < 0.05 of 10: previous in d, p = 0.005. |
| H8a | Supported. Previous encrypt rises 0.0424 s per iteration ($R^2$ 0.99996; 3.84 → 16.10 s in a), decrypt 0.052 (a) / 0.063 (b) s per iteration. Proposed is flat: 8.24–8.32 s encrypt, fitted change ≤ 0.2%. |
| H8b | Supported for proposed (fitted change ≤ 0.32%). Previous: no significant slope (p ≥ 0.06), but decrypt time moves between 1.46 and 3.45 s with $a$ at a fixed size, and the $a = 1$ row is fastest (encrypt 4.74 s against ≈ 5.5 s). |
| H8c | Supported. Proposed encrypt rises from 4.17 s (16-bit primes) to 8.38 s (1024-bit), **convexly**: quadratic $R^2$ 0.998 against 0.960 for a line, so do not describe it as linear. Previous is flat (5.09–5.12 s). |

**Timing structure of the previous pipeline (unverified explanation).** Its residuals are large and
repeat across operations: the same files run slow or fast in both encrypt and decrypt (e.g.
`0091-pptx.pptx`, 7.69 MB: encrypt 11.2 s against a fitted 13.4 s, decrypt 3.0 s against 7.0 s), and in
c/d its time changes with $a$ at a fixed size. A likely cause is that the byte-level ACM gathers and
scatters through a permutation table 50 times, so its cache behaviour depends on the map for each exact
length and key. This has not been tested.

### 8.5 Recommended claims: uniformity (H1, H4)

Entropy and χ² are randomness tests of one kind: they test whether the bytes are **uniform**, i.e.
whether every byte value occurs equally often. Neither is image-specific; both treat the plainfile
and the cipherimage as byte sequences, which is why the plainfile baseline is part of the result.
Present them as follows.

**Claim (H1 + H4).**

> For all seven file types and both pipelines, the cipherimage byte histogram is statistically
> indistinguishable from uniform: each type passes the χ² test (df 255, α = 0.05) under a per-type
> binomial acceptance rule, and the pooled failure rate (34 of 691 files, proposed) matches the 5%
> expected of an ideal cipher. The same test rejects every plainfile (0 of 691 pass), including
> compressed formats whose entropy is already close to 8 bits.

Support: §8.4 H1 and H4, `f_decisions.csv`, `f_plain_vs_cipher.png`, `f_chi2_pvalues.png`
(p-values uniform per type).

**How to frame it.**

- **Name the property tested: byte-frequency uniformity.** Avoid an unqualified "passes the
  randomness test" or "is random". These tests see only how often each value occurs, not order,
  position or correlation (§10). A reader will compare an unqualified randomness claim with suites
  such as NIST SP 800-22, which were not run.
- **Treat entropy and χ² as one test reported two ways, not two independent tests.** Both come from
  the same histogram ($\chi^2 \approx 2F\ln 2\,(8 - H)$) and failed on exactly the same files.
  Report entropy because image-encryption papers report it; base the decision on χ², which has a
  significance threshold that does not depend on size.
- **State the result as a failure to reject.** Say "indistinguishable from uniform" or "consistent
  with uniform", not "proven uniform". Explain that about 5% of files failing is expected at
  α = 0.05, so 34 failures support the claim rather than weaken it.
- **Lead the plainfile contrast with χ², not entropy.** Compressed plainfiles (MP4 averages 7.95
  bits, MP3 7.90) look nearly uniform by entropy. χ² still rejects every one of them by a wide
  margin (lowest MP4 value 2.9×10⁴, lowest MP3 1.9×10⁴; corpus minimum 685, and 599 of 691 files
  above 10⁴). Entropy alone would suggest the input was already almost
  random.
- **Credit RT for the uniformity, not ACM.** A permutation does not change a histogram (§5.3). The
  uniformity comes from RT's fresh $r_i$, so it holds whatever the key or ACM parameters (H7).
- **Pair it with H2 for an "output looks random" section.** H1 shows each cipherimage is uniform.
  H2 (NPCR_ch, UACI) shows two encryptions of the same file differ like two independent uniform
  images. Together they cover single-image and between-image statistics. The limitation that no
  byte-pair or correlation test was run (§10) belongs next to them.
- **Pipeline parity (H5):** proposed and previous are indistinguishable here (657 and 656 of 691
  pass). The proposed design is no worse on uniformity; it is not better either.

---

## 9. Implementation checks

Beyond the per-configuration round-trip check (§3), the implementation was checked as follows on
2026-10-16 (macOS, `-O0`).

- **ACM mapping** (`ftie/acm.cpp`). A test including `acm.cpp` compared both mappings against a 64-bit
  reference $A^n \bmod N$: 0 mismatches and 0 non-bijective maps over 1,196 random keys (including
  $ab > 2^{31}$) across every even $N$ in 4–600. For $a = b$, `mapping_equal` = `mapping_general` =
  reference in 897 cases with $a$, $n$, $N$ mixed in one process. Full encrypt/decrypt round trips at
  $N$ = 38, 74 and 362 succeed in both pipelines.
- **NPCR_ch and UACI thresholds.** 2,000 re-encryptions on the real proposed pipeline give reject rates
  at the nominal 5% (§4.8).
- **Avalanche** (`calculate_avalanche`). On a 3.2 MB cipherimage pair it gives 0.50003, matching an
  independent decode of the pixels.
- **Corpus input check.** A corrupted manifest hash gives a FAILED line and no row (§6.3).
- **Frozen keystream.** A tampered `f.csv.keystream` stops a resumed f run (§6.2).
- **Interrupted a–e runs.** Each experiment writes `<letter>.csv.partial`, renamed to `<letter>.csv`
  only on exit 0. An interrupted run leaves only the `.partial`, and a plain re-run redoes that
  experiment with unchanged keys (keys are regenerated only with `--new-params`).

Seed chaining was checked on 2026-11-02 (macOS, `-O0`):

- **Next seed.** The CLI's printed next seed for a 50,000-byte file equals $x_{L'}$ computed
  independently in Python ($L' = 3N^2/2$ squarings of $s$ mod $pq$). Two messages encrypted in sequence
  (the second from the first's next seed) each decrypt byte for byte with their own seed; the second
  does not decrypt with $s$.
- **a–e.** The first two rows of every experiment: pinned $a, b$; four distinct seeds per row; row 1's
  `seed_c1_proposed` is the key's $s$, row 2's is row 1's `seed_next_proposed` (a–d); each e row
  starts from its own key's $s$.
- **f resume.** A three-file run done in three invocations (one file; then with one file's manifest
  hash corrupted, which FAILED; then all three) gives one unbroken chain in CSV order starting at $s$.
  A first-row seed that is not $s$, and a torn last row, each stop the resumed run.
- **Analysis.** `analyze.py`'s seed-chain check passes on that output and reports a broken link, a
  reused seed and a wrong start when each is introduced. Run on the 2026-10-16 data converted to the
  new columns, it completes, and its ACM identity flag finds 10 of a's 30 rows ($P = 30$) and 3 files
  in f ($P = 10$).
- **Thresholds with chaining.** 2,000 chained re-encryptions give 5.30% (NPCR_ch), 5.75% (UACI) and
  5.35% (avalanche, naive SD) at nominal 5% (§4.6, §4.8).

---

## 10. Limitations (all experiments)

- **What NPCR/UACI/avalanche measure here.** Because RT adds fresh randomness per byte, the
  differences between cipherimages come from $r$, not from plainfile diffusion. Flipping one plainfile
  bit deterministically changes only the two RT bytes of that position; ACM moves them but does not
  spread the change. So avalanche here measures randomized re-encryption, not the classical
  avalanche/diffusion property. Describe it that way.
- **Pair structure is outside the metric set.** For every plainfile byte,
  $C_{2i} - C_{2i+1} \equiv P'_i - K_i \pmod{256}$, independent of $r$. Histogram tests (entropy,
  χ²) and NPCR/UACI cannot see this. In the previous pipeline the same $K$ is used for every input
  within an experiment; in the proposed pipeline each encryption has its own $K$ (§2.3). A reviewer may
  ask about byte-pair or correlation tests; state the scope of the metric set.
- **Randomness source.** Cipherimage uniformity rests on `std::random_device` (on Linux/libstdc++,
  the hardware RNG or `/dev/urandom`). The platform is recorded in `env.txt`.
- **Random plainfile in a–e.** Their quality metrics say nothing about behaviour on structured input;
  f (corpus) covers that.
- **Representativeness of f.** It is unknown whether Tiny's 0001–0100 is a representative sample of
  each subset; the NapierOne paper does not say how files were ordered.
- **Timing.** Each value is one wall-clock sample of a `-O0` build, so there is no estimate of
  repeat-to-repeat noise, and absolute times would be lower with optimisation. The previous pipeline's
  timing also depends on more than size (§8.4); its fitted s/MB is an average, not a law.
- **Avalanche has no pass rule** (§4.6). Its null SD is checked on real chained re-encryptions for the
  proposed pipeline; the previous pipeline's $SD_0$ is checked only by a Monte Carlo of the RT
  equations.
- **Seed chain per experiment.** Each experiment restarts the chain at the key's $s$ (§3), so the first
  encryption of a, b, c, d and f uses the same $K$. Within an experiment no two encryptions share $K$,
  which is what the pair statistics need, but across experiments the run does not follow one continuous
  sender–receiver chain.
- **Previous FTIE results.** The original eight test files are unpublished, so earlier FTIE figures
  cannot be reproduced; f's recorded NapierOne list addresses this for the new results only.

---

## 11. EC2 run procedure (all experiments)

The step-by-step commands, from SSH login to commit, are in [RUNBOOK.md](RUNBOOK.md). The principles
behind them:

- **Code committed and pushed first**, so `env.txt` records a clean commit (`git_dirty=no`).
- **Instance:** Ubuntu 24.04, **not burstable** (avoid t3/t4g: CPU credits distort timing; c7i.large
  is enough), 20 GB disk. The code is single-threaded. Nothing else runs on the instance, and a–e and f
  run one after the other, not in parallel, so they do not disturb each other's timing.
- **Corpus and manifest prepared before the long run**, and a smoke test on three files, so the
  ≈ 5.6 h run is unattended: `./run_experiments.sh ; ./run_corpus_experiment.sh`.
- **One key file.** Both scripts generate `bbs_params.csv` only if it is missing (the smoke test
  creates it), so a–e and f share it.
- **f's frozen key restored** (re-run): `f.csv.params` is committed; the gitignored `f.csv.keystream`
  is copied to the instance and checked against its recorded hash.
- **Old results removed once before the run** (`rm experiment/results/[a-e].csv
  experiment/results/f.csv experiment/results/f.stderr.log`). Then the run command doubles as the
  resume command after any interruption: a–e redo only experiments without a finished CSV, f skips
  files already in `f.csv` and continues the seed chain from its last row.
- **Checks before committing:** five a–e experiments with exit 0 and 31 lines each, no `.partial`
  files, empty a–e stderr logs, f rows = manifest files, no FAILED or NaN, `git_dirty=no`, and an
  unbroken seed chain in a–d and f.

The 2026-10-16 run followed this procedure (without the seed-chain items) and passed every check
(re-verified by the integrity step of `experiment/analyze.py`, §8.2).

---

## 12. Resources (2026-10-16 run)

- **Time.** a–e took 2,090 / 2,140 / 1,515 / 1,530 / 1,158 s, **≈ 2.3 h** in total (`run.log`). f
  started at 18:07:50 UTC; its end time was not recorded (`f.stderr.log` has no timestamps), but its
  timed encryptions and decryptions alone add up to 4,492 s for 672.5 MB.
- **Memory.** Peak resident memory for a 10,000,000-byte file was **432 MB** (sizing run on macOS);
  2 GB is enough.
- **Disk.** f's four scratch files per corpus file total about 6× its size (cipherimages are ≈ 2.03×
  the plainfile), so ≈ **4 GB**; a–e add ≈ 100 MB; plus the downloaded archives and extracted files.

---

## 13. Requirements traceability (`new-experiment.md`, f corpus)

| Req. | Status in code |
| --- | --- |
| D1 Loader | Done (`manifest.cpp`, `f.cpp`) |
| D2–D6 Coverage, exclusions, tier, selection, independence | Enforced by `build_manifest.sh` and the archive choice |
| D7 Provenance | Manifest fields; size and SHA-256 re-checked at run time (§6.1) |
| D8 Attribution | Paper-side |
| M1 UACI | Done, in all experiments |
| M2 χ² | Done, pooled R+G+B, in all experiments |
| M3 Per-image thresholds | Done, with RT-pair sample size (§4.8) |
| M4 Pair definition | Done; bit 1 of byte 0 everywhere |
| M5 Padding in sample | Answered: all $3N^2$ bytes are RT output (§2.2) |
| M6 Existing metrics | Kept; avalanche on decoded pixels (§4.6) |
| M7 Plaintext baseline | Done |
| O1–O3 Row per file, columns, both pipelines | Done |
| O4 Per-type summary | Done (`experiment/analyze.py`, §8) |
| O5 Run record | Done (§7.1) |
| O6 Resumable | Done, by SHA-256, with frozen keystream and the running seed restored from the last row (§6.2) |
| O7 No overwrite of old results | `f.cpp` refuses to append to a CSV with another header (§6.3) |
| O8 Timing from corpus | Done |

---

## 14. References

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
