# FTIE Experiments — Methodology

Status: **experiment f not yet run; experiments a–e already run** (§8). §1–§7 describe the corpus
experiment f exactly as implemented in
`experiment/f.cpp`, `experiment/tools.cpp`, `experiment/manifest.cpp` and `run_corpus_experiment.sh`
(staged on `master` on top of `c7ca275`, not yet committed), and the FTIE code in `ftie/`. Where the
implementation differs from the requirements in `new-experiment.md`, the implementation is described
and the difference is flagged.

It has two jobs:

1. Source material for the paper's Methodology section: corpus experiment f (§1–§7) and the
   parameter sweeps a–e (§8).
2. A pre-flight check before the EC2 run (§9–§11). **Read §9 first** — it lists issues found while
   writing this document, some of which should be settled before launching.

---

## 1. Objective

Measure the statistical quality and cost of FTIE (File-To-Image Encryption) on **real files** of the
seven file types used in the original FTIE evaluation. It replaces the old synthetic size sweep (the
previous experiment f), which, like the parameter sweeps a–e (§8), encrypted uniformly random bytes
and therefore measured entropy, NPCR and avalanche on plaintext that was already uniform.

For each corpus file and for each of two pipelines (current and deprecated), the experiment records:

- cipherimage randomness: Shannon entropy, Pearson χ²;
- randomization between two encryptions of the same file: NPCR (two variants) and UACI;
- sensitivity to a one-bit plaintext change: avalanche;
- encryption and decryption wall-clock time;
- a plaintext baseline: entropy and χ² of the original file's bytes;
- per-file critical values for NPCR, UACI and entropy at α = 0.05.

The FTIE algorithm itself (BBS, RT, ACM) is not changed by this experiment.

---

## 2. System under test

Two pipelines are run on every file. Both share the same padding and the same RT step; they differ
in the keystream source and in what ACM permutes.

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

Consequence for the metrics (answers requirement M5): **every one of the $3N^2$ cipherimage bytes is
RT output.** There are no unencrypted padding pixels; the padding (zeros + length) is encrypted
along with the file. For a 13.9 KB file the padding is about 3% of $P'$.

### 2.3 Keystream

- **Current pipeline — Blum Blum Shub** (`bbs.cpp`). $m = pq$, $x_0 = s$,
  $x_i = x_{i-1}^2 \bmod m$, $K_i = x_i \bmod 256$ (low 8 bits of each state), for
  $i = 1..L'$. Before generating, the code checks $p, q$ prime (GMP Miller–Rabin, 40 rounds),
  $p \equiv q \equiv 3 \pmod 4$, $1 < s < m$, $\gcd(s, m) = 1$.
- **Deprecated pipeline — literal keystream.** 10,000,000 bytes from `std::random_device`,
  generated once per process start, indexed as $K_{i \bmod 10^7}$.

### 2.4 Randomized Text, RT (`rt.cpp`)

For each padded plaintext byte $P'_i$, with a fresh $r_i$:

$$C_{2i} = K_i + 2P'_i + r_i \pmod{256}, \qquad C_{2i+1} = 2K_i + P'_i + r_i \pmod{256}$$

Decryption: $(C_{2i} - K_i) - (C_{2i+1} - 2K_i) \equiv P'_i$. RT doubles the byte count and makes
encryption **non-deterministic**: the same file encrypted twice under the same key gives different
cipherimages, differing only through $r$.

### 2.5 Packing and Arnold's Cat Map

**Current pipeline** (`ftie.cpp`, `acm.cpp`): $C$ is packed row-major into an $N \times N$ RGB image
(pixel $j$ = $(C_{3j}, C_{3j+1}, C_{3j+2})$, $x = j \bmod N$, $y = \lfloor j/N \rfloor$), then the
**pixels** are permuted with

$$A = \begin{pmatrix} 1 & a \\ b & 1+ab \end{pmatrix}, \qquad M = A^n \bmod N, \qquad
\text{cipher}(x, y) = \text{plain}\big(M (x, y)^\top\big)$$

computed by modular matrix exponentiation (`mapping_general`); when $a = b$ a closed-form Fibonacci
mapping is used instead (`mapping_equal`). $M$ is applied once.

**Deprecated pipeline** (`acm.cpp: ftie::deprecated::acm`): the cat map is applied to the **byte
array** $C$ (length $T = 3N^2$) laid on a $G \times G$ grid, $G = \lfloor\sqrt{T}\rfloor + 1$, keeping
only targets $< T$; the base map is iterated $n$ times. The permuted bytes are then packed into the
$N \times N$ image.

Both pipelines produce cipherimages of the same size for the same file. Output is an 8-bit RGB PNG
written by png++/libpng with default settings (zlib compression, adaptive row filters).

### 2.6 Parameters used in this experiment

| Parameter | Value | Source |
| --- | --- | --- |
| BBS $p, q$ | 1024-bit primes, $\equiv 3 \pmod 4$ (modulus ≈ 2048 bits) | last row of `experiment/bbs_params.csv`, produced by `make experiment-gen-params` |
| BBS $s$ | random in $[2, m)$, coprime to $m$ | same row |
| ACM $a, b$ | each uniform 16-bit from `std::random_device` | drawn by `f.cpp` on first invocation |
| ACM $n$ | 50 | fixed in `f.cpp` |
| Deprecated keystream | 10,000,000 random bytes | regenerated at each process start (see §9, F10) |

$p, q, s, a, b, n$ are drawn once and **frozen** in `experiment/results/f.csv.params`; every file uses
the same key, including after a resume. The same key is used for every file and both pipelines' ACM.

---

## 3. Dataset

### 3.1 Source

NapierOne (Davies, Macfarlane and Buchanan, *Forensic Science International: Digital Investigation*
40 (2022) 301330, doi:10.1016/j.fsidi.2021.301330). Cite the published version, not the arXiv
preprint, and keep the licence/attribution terms the paper requires (its §3.2, §5.2).

### 3.2 Subsets and selection

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

- **Tier:** the *Tiny* archive of each subset (sequence numbers 0001–0100). No random draw: the file
  list is the archive's fixed list, so a reader can obtain the same files.
- **Size cap:** files up to 10 MB at their natural size. Larger files are future work.
- **Excluded:** the subset documentation PDF shipped inside each archive (e.g. `DOCX.pdf`); PSEUDO,
  PURE, `-PASSWORD`, `-NOMAGIC`, EXTRA (ransomware) and ENCRYPTED subsets; JPG QUALITY-\* subsets.
- **Independence:** one subset per type, so no sequence number is drawn from two related subsets.

### 3.3 Expected sample (from the S3 listing of the Tiny archives; sizes only, not yet verified by hash)

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

The MP4-FROM-WEB Tiny archive does not match the paper's Table 5 for that subset (median 1.45 MB vs
a stated average of 312 KB), and 82 of its 100 file sizes also occur in the MP4 Tiny archive. Report
this as observed; it was checked by size only.

### 3.4 Manifest and provenance

`f.cpp` reads a CSV manifest (default `experiment/corpus_manifest.csv`):

```
type,subset,file_name,path,size_bytes,sha256,archive_tier,download_date
```

One row per file, in the order the files will be processed. The manifest is the provenance record
(requirement D7): subset, file name, size, SHA-256, tier, download date. **The manifest file does not
exist yet and there is no script that builds it** (§9, F5). Fields are split on commas without quoting,
so no field may contain a comma.

---

## 4. Procedure

`run_corpus_experiment.sh [manifest] [output]` builds `bin/exp-f.o` and runs it. For each manifest row,
in order:

1. **Resume check.** If the file's SHA-256 already appears in the output CSV, skip it.
2. **Build the pair.** $P_1$ = the corpus file. $P_2$ = $P_1$ with **bit 0 of byte 0 flipped**
   (`byte[0] ^= 0x01`), written to `experiment/dum/`.
3. **For each pipeline** (deprecated first, then current):
   1. Time $\text{Enc}(P_1) \to C_1$.
   2. Time $\text{Dec}(C_1) \to R$. If $R \ne P_1$ byte-for-byte, the file is recorded as FAILED
      (no row written) and retried on the next invocation.
   3. $\text{Enc}(P_1) \to C_2$ (a second, independent encryption of the same file).
   4. NPCR (any-channel), NPCR (per-channel) and UACI between $C_1$ and $C_2$.
   5. Entropy and χ² of $C_1$.
   6. $\text{Enc}(P_2) \to C_2$; avalanche between $C_1$ and this $C_2$.
4. Read $W, H$ from the current pipeline's $C_1$.
5. Entropy and χ² of $P_1$'s raw bytes (plaintext baseline).
6. Per-file critical values from $W, H$.
7. Append one row to the output CSV and flush.

All encryptions of one file in one pipeline use the same frozen key. Each pipeline therefore performs
3 encryptions and 1 decryption per file; only the first encryption and the decryption are timed.

### 4.1 Pairs used by each metric (requirement M4)

| Metric | Pair | What differs between the two cipherimages |
| --- | --- | --- |
| NPCR, NPCR_ch, UACI | $\text{Enc}(P_1)$, $\text{Enc}(P_1)$ | only the RT random bytes $r$ |
| Avalanche | $\text{Enc}(P_1)$, $\text{Enc}(P_2)$ | $r$, plus one plaintext bit |

This NPCR/UACI pair is **not** the classic one-pixel-plaintext-change pair of Wu et al.; it measures
how different two encryptions of identical input are. State this in the paper.

---

## 5. Metrics (as implemented in `experiment/tools.cpp`)

All cipherimage metrics are computed on the decoded RGB pixels of an $N \times N$ image unless noted;
$F = 3WH = 3N^2$ is the number of channel samples.

### 5.1 Entropy — `calculate_entropy`, `calculate_entropy_bytes`

Shannon entropy of the 256-bin histogram of the R, G and B bytes **pooled**:

$$H = -\sum_{k=0}^{255} p_k \log_2 p_k, \qquad p_k = O_k / F$$

Ideal: 8 bits. The plaintext baseline (`entropy_plain`) uses the same formula over the file's $L$
raw bytes.

### 5.2 Chi-square — `calculate_chi_square`, `calculate_chi_square_bytes`

Pearson χ² over the same pooled histogram, expected count $E = F/256$ per bin, 255 degrees of freedom:

$$\chi^2 = \sum_{k=0}^{255} \frac{(O_k - E)^2}{E}$$

Critical value 293.25 at α = 0.05 (310.46 at α = 0.01). Valid only when $E \ge 5$, i.e. at least 1,280
bytes; this is **not enforced** in code and must be applied in analysis. `chi2_plain` uses the file's
raw bytes. Entropy and χ² come from the same histogram and are approximately linked by
$\chi^2 \approx 2F\ln 2\,(8 - H)$, so they are not independent evidence.

### 5.3 NPCR, any-channel — `calculate_npcr` (column `npcr_*`, kept for continuity with a–e and the old f)

$$\text{NPCR} = \frac{1}{N^2}\sum_{x,y} \mathbb{1}\big[C_1(x,y) \ne C_2(x,y)\big]$$

where a pixel counts as different if any channel differs. Fraction in $[0,1]$. This is not the
literature definition and its ideal is not 99.6094% (§9, F7).

### 5.4 NPCR, per-channel — `calculate_npcr_channel` (column `npcr_ch_*`)

$$\text{NPCR}_{ch} = \frac{1}{3N^2}\sum_{c \in \{R,G,B\}}\sum_{x,y} \mathbb{1}\big[C_1^c(x,y) \ne C_2^c(x,y)\big]$$

Mean of the three channel NPCRs. Fraction in $[0,1]$. Ideal for independent uniform images:
$255/256 = 0.996094$.

### 5.5 UACI — `calculate_uaci`

$$\text{UACI} = \frac{100\%}{3N^2}\sum_{c}\sum_{x,y}\frac{|C_1^c(x,y) - C_2^c(x,y)|}{255}$$

**Percent** in $[0,100]$ (unlike the other metrics, which are fractions). Ideal: $257/768 = 33.4635\%$.

### 5.6 Avalanche — `calculate_avalanche`

Fraction of differing bits between the two **PNG files' bytes**, compared byte by byte up to the
shorter file's length. Ideal 0.5. Note that this is computed over the compressed PNG container, not
the decoded pixels (§9, F8).

### 5.7 Timing — `chrono_encrypt`, `chrono_decrypt`, `chrono_deprecated_*`

Wall-clock time (`std::chrono::high_resolution_clock`) around one full call of encrypt or decrypt.
Includes: reading the input file, BBS parameter validation (two 40-round Miller–Rabin tests on
1024-bit numbers, current pipeline only), keystream generation, RT, ACM, PNG encode/decode with
zlib, and writing the output. One measurement per file per operation. Execution time vs size
(previously the synthetic size sweep in the old f) is read from `size_bytes` against `enc_*_s` / `dec_*_s`.

### 5.8 Per-file critical values (requirement M3)

Computed from each cipherimage's own $W, H$ (so the threshold tightens as files grow):

| Column | Formula in code | Test |
| --- | --- | --- |
| `npcr_crit` | $\mu - 1.645\sqrt{\mu(1-\mu)/F}$, $\mu = 255/256$ | fail if `npcr_ch` < value |
| `uaci_crit_low`, `uaci_crit_high` | $33.4635 \mp 1.96\sqrt{0.05599/F}$ | fail if `uaci` outside range — **units bug, §9 F1** |
| `entropy_crit` | $8 - 293.25 / (2F\ln 2)$ | fail if entropy < value |
| (none; constant) | 293.25 | fail if χ² > value |

Reference values for $F$ (cipherimage) at three file sizes, with the UACI range in correct percent
units:

| Plainfile | $N$ | `npcr_crit` | UACI range (correct units) | `entropy_crit` |
| --- | --- | --- | --- | --- |
| 13.9 KB | 98 | 99.548% | 33.19–33.74% | 7.99239 |
| ~1 MB | ~818 | 99.602% | 33.43–33.50% | 7.99989 |
| 10 MB | 2582 | 99.607% | 33.45–33.47% | 7.99999 |

---

## 6. Output

### 6.1 Result CSV (`experiment/results/f.csv`, committed)

One row per corpus file (requirement O1/O2). `_dep` = deprecated pipeline, `_cur` = current.

| Columns | Meaning |
| --- | --- |
| `type, subset, file_name, sha256, size_bytes` | from the manifest |
| `width, height` | cipherimage size |
| `entropy_plain, chi2_plain` | plaintext baseline |
| `npcr_dep, npcr_cur` | any-channel NPCR (fraction) |
| `npcr_ch_dep, npcr_ch_cur` | per-channel NPCR (fraction) |
| `uaci_dep, uaci_cur` | UACI (percent) |
| `entropy_dep, entropy_cur, chi2_dep, chi2_cur` | cipherimage $C_1$ |
| `avalanche_dep, avalanche_cur` | file-byte avalanche (fraction) |
| `enc_dep_s, dec_dep_s, enc_cur_s, dec_cur_s` | seconds |
| `npcr_crit, uaci_crit_low, uaci_crit_high, entropy_crit` | α = 0.05 per-file thresholds |

### 6.2 Run record (`experiment/results/f.csv.params`, committed)

Written once by `f.cpp`: `bits, p, q, s, acm_a, acm_b, acm_n, manifest` (manifest **path**, not
contents). Appended/refreshed by `run_corpus_experiment.sh` on every invocation: `git_commit`
(`git rev-parse HEAD`), `compiler` (`g++ --version`), `run_date`.

### 6.3 Scratch files

Per file, `experiment/dum/f_<sha256[0:12]>_{plainfile_2, cipherimage_1, cipherimage_2, roundtrip_1}`.
They are never deleted. After a file finishes, `cipherimage_1` holds the current pipeline's
$\text{Enc}(P_1)$ and `cipherimage_2` its $\text{Enc}(P_2)$, so current-pipeline metrics can be
recomputed later from them; the deprecated pipeline's cipherimages are overwritten.

---

## 7. Analysis plan

Not implemented in code yet (requirement O4 has no script). Planned:

1. **Filter.** Exclude from χ² and entropy any file whose sample is under 1,280 bytes (plaintext for
   `*_plain`, $F$ for cipherimages). Report excluded files by name.
2. **Per-file test** at α = 0.05: NPCR_ch (one-sided), UACI (two-sided), entropy (one-sided), χ²
   (one-sided), each against its per-file critical value. Use corrected critical values (§9 F1, F2).
3. **Per-type decision.** Count per-file failures per type and metric. Accept the type if failures do
   not exceed the binomial 99th percentile at $p = 0.05$:

   | Files in type ($n$) | Accept if failures ≤ |
   | --- | --- |
   | 94 (PPTX) | 10 |
   | 98 (PDF, ≤ 10,000,000 B) | 10 |
   | 99 (PDF, ≤ 10 MiB) | 11 |
   | 100 | 11 |

   This keeps each type's false-reject probability under 1% per metric; across 7 types the chance of
   at least one false reject on a metric is about 7%. PPTX and PDF fall slightly below the
   $\min(n\alpha, n(1-\alpha)) \ge 5$ rule of thumb (NIST/SEMATECH e-Handbook §7.2.4); report this.
4. **Descriptive statistics** per type and pipeline: $n$, mean, SD, min, max of every metric.
   Compare entropy across sizes by pass/fail and by shortfall $8 - H$, not raw $H$.
5. **Timing:** scatter of time vs `size_bytes` per pipeline; fit (e.g. linear) and report the slope in
   s/MB.
6. **Plaintext vs cipherimage:** contrast `entropy_plain`/`chi2_plain` with the cipherimage values per
   type, to show the cipher output is uniform regardless of input structure.

---

## 8. Parameter-sweep experiments (a–e)

Status: **already run** (2026-08-30; `experiment/results/a.csv`–`e.csv`, committed). Not part of the
EC2 run. They answer a different question from f: how each pipeline behaves as **one FTIE parameter**
varies, which f does not cover because it fixes $a$, $b$, $n = 50$ and 1024-bit BBS primes. The old
synthetic size sweep that used to be experiment f is retired; its results are kept as
`experiment/results/f_size_sweep.csv`.

### 8.1 Common setup (`experiment/a.cpp`–`e.cpp`, run by `run_experiments.sh`)

| Item | Value |
| --- | --- |
| Plaintext | 3,210,000 random bytes from `std::random_device`, generated once per experiment and reused for every row. $P_1$ has byte 0 = `0x01`, $P_2$ has byte 0 = `0x03` (bit 1 flipped). Cipherimage $N = 1464$ |
| Deprecated keystream | 10,000,000 random bytes (e: 3,210,000) |
| BBS | last row of `bbs_params.csv` (1024-bit primes), except e |
| Pipelines | deprecated and current, same plaintext |

Per row, for each pipeline: time $\text{Enc}(P_1) \to C_1$; time $\text{Dec}(C_1)$, **written back over
$P_1$**; $\text{Enc}(P_1) \to C_2$; any-channel NPCR($C_1, C_2$) (§5.3); entropy($C_1$) (§5.1);
$\text{Enc}(P_2) \to C_2$; avalanche($C_1, C_2$) on PNG file bytes (§5.6). One timing sample per row.
Rows go to stdout, and `run_experiments.sh` writes them to `<letter>.csv` with columns
`x, npcr_dep, entropy_dep, avalanche_dep, enc_dep_s, dec_dep_s, npcr_cur, entropy_cur, avalanche_cur,
enc_cur_s, dec_cur_s`. There is no UACI, χ², per-channel NPCR or plaintext baseline.

### 8.2 What each experiment varies

| Exp. | Swept (30 rows) | Held fixed | Current-pipeline ACM path |
| --- | --- | --- | --- |
| a | $n$ = 10, 20, …, 300 | $a = b$, random 16-bit | `mapping_equal` (Fibonacci) |
| b | $n$ = 10, 20, …, 300 | $a \ne b$, random 16-bit | `mapping_general` |
| c | $a = b$ = 1, 2185, …, 63337 (step 2184) | $n = 50$ | `mapping_equal` |
| d | $a$ = 1, 2185, …, 63337; $b = a + 1$ | $n = 50$ | `mapping_general` |
| e | BBS row: primes 16 → 1024 bits (modulus ≈ 32 → 2048 bits) | $a, b$ random, $n = 50$ | `mapping_general` (unless $a = b$) |

In e the `x` column (headed `bits`) holds the modulus $m = pq$ as a decimal number, not a bit count.
The deprecated pipeline never uses BBS, so in e it acts as a control. Its ACM always uses the
iterated base map, so $n$ costs it linear time, and it has no $a = b$ special case.

### 8.3 What they can and cannot show

**The quality metrics cannot respond to the swept parameter, by construction.**

- **ACM parameters (a–d).** Entropy and χ² come from a histogram, which a permutation does not change.
  NPCR, UACI and pixel-level avalanche compare two cipherimages permuted by the *same* map, so their
  sums do not change either. Changing $a$, $b$ or $n$ changes which permutation is applied, and none
  of these metrics can see that.
- **Key size (e).** RT's fresh $r_i$ makes every cipher byte uniform whatever $K$ is, so a weak
  32-bit modulus and a 2048-bit one give the same histogram and differences.

The results agree. Across all 150 rows (current pipeline), NPCR stays at 0.999977–0.999990, entropy at
7.99997–7.99998, and avalanche at 0.49913–0.49947. The scatter is noise around the RT-determined
values (§9 F7, F8).

**Timing is the informative output**, and it does not depend on plaintext content (BBS, RT and ACM
cost depends only on length), so the random plaintext does not weaken it:

| Exp. | Deprecated encrypt (s) | Current encrypt (s) | Reading |
| --- | --- | --- | --- |
| a | 3.7 → 15.9 | 8.9 – 10.0 | deprecated cost grows linearly with $n$ (map iterated $n$ times); current computes $A^n$ once |
| b | 3.7 → 15.9 | 8.2 – 8.5 | same, general path |
| c | 4.5 – 5.4 | 9.1 – 9.2 | flat in $a$ |
| d | 4.5 – 5.5 | 8.2 – 8.3 | flat in $a$ |
| e | 5.35 – 5.38 | 4.0 → 8.3 | current cost grows with modulus size; deprecated (no BBS) flat |

### 8.4 Issues found in a–e

- **c does not actually sweep $a$ in the current pipeline.** The Fibonacci cache in `acm.cpp:62` is
  a `static` map keyed only on the index, not on $a$ or $N$. With $n = 50$ fixed, the values cached
  for the first row ($a = b = 1$) are reused for all 30 rows. Its current-pipeline column therefore
  measures the $a = b = 1$ map 30 times. The deprecated pipeline is unaffected. a is unaffected,
  because $a$ and $N$ are fixed there while $n$ varies.
- **d's last 8 rows ($a \ge 48{,}049$) use a different matrix.** $ab > 2^{31} - 1$, so `1 + a*b`
  overflows (§9 F3). Emulating the `-O0` behaviour at $N = 1464$, the code's $A_{11}$ differs from
  $1 + ab \bmod N$, and $\det A \equiv 1369 \pmod{1464}$ instead of 1. 1369 is coprime to 1464, so
  the map is still a permutation and decryption still worked, but it is not the documented cat-map
  matrix. (Emulated, not re-run.)
- **The random $a, b$ in a, b and e are not recorded**, so whether those runs hit the overflow (about
  15% per draw) or, in e, drew $a = b$ cannot be checked now.
- **No round-trip check.** Decryption overwrites $P_1$ in place. A wrong decryption would have silently
  changed the plaintext for every later row, without failing. Timing would be unaffected.
- Any-channel NPCR (§9 F7) and file-byte avalanche (§9 F8) apply here unchanged.
- Rows within an experiment reuse one plaintext and one key, so they are 30 measurements of one
  configuration each, not independent samples. Each timing is a single sample.

### 8.5 How to use a–e in the paper

- Report them as **execution-cost results**: time vs ACM iterations (a, b), vs ACM matrix entry
  (c, d), and vs BBS modulus size (e), with the deprecated pipeline as a comparison/control.
- Do **not** use their entropy/NPCR/avalanche columns as evidence that a parameter affects security.
  At most they show that the metrics stay at their RT-determined values whatever the parameter, which
  is expected from §8.3.
- State the plaintext (3.21 MB random), the single-sample timing and the build (`-O0`).
- If c's current-pipeline timing is reported as a function of $a$, add the cache caveat. A valid
  sweep needs the cache keyed on $(a, N, \text{index})$, which is an ACM code change and outside the
  current requirements.
- No re-run is needed for the timing results, apart from the c/d caveats above.

---

## 9. Pre-flight findings

Found while reading the code and checking it against the local validation run
(`experiment/dum/validation_g.csv`, 4 repo files, macOS). Grouped by when they need attention.

### Must handle before or during the run

**F1 — UACI critical range is 100× too narrow (`tools.cpp:240-246`).** The mean is in percent
(33.4635) but the half-width $1.96\sqrt{0.05599/F}$ is in fraction units. The validation run shows
33.4568–33.4702 for a 40×40 image; the correct range is 32.79–34.13. As written, almost every file
would fail UACI. The columns are pure functions of $W, H$, so this can also be fixed in analysis by
recomputing; either way, do not report `uaci_crit_*` as written.

**F2 — The NPCR_ch and UACI tests assume independent bytes, but RT bytes move in pairs.** Between two
encryptions of the same file, $C_{2i}$ changes if and only if $r_i$ changes, and so does $C_{2i+1}$.
So the $3N^2$ per-channel indicators are really $1.5N^2$ pair indicators counted twice.

- NPCR_ch: the true SD is $\sqrt{2}$ times the one in `npcr_crit`. At nominal α = 0.05 the real
  per-file reject rate is **12.2%**, so an ideal 100-file type averages about 12 failures against an
  acceptance limit of 11, and fails the type-level rule with probability **≈ 0.57**.
- UACI: a simulation of the RT pair gives a within-pair correlation of about 0.40 for $|ΔC|$, so the
  true SD is ≈ 1.18× the formula's. Real per-file reject rate ≈ **9.8%**; an ideal type fails with
  probability ≈ 0.27.

This applies to both pipelines (the sum is unchanged by any permutation). Fix in analysis: use
$F_{\text{eff}} = 1.5\,WH$ for NPCR_ch; for UACI either use the variance inflation $(1+\rho)$ or
compute the statistic per RT pair. The raw metric values are unaffected; only the thresholds change.
Settle this before writing up pass/fail claims.

**F3 — ACM key draw can overflow and break decryption for some image sizes (`acm.cpp:95`).** In
`A[1][1] = (1 + a * b) % N`, the `uint16_t` operands are promoted to `int`, so $ab > 2^{31}-1$ is
signed overflow (undefined behaviour). With random 16-bit $a, b$ this happens in about **15%** of
draws. Built at `-O0` (the Makefile's default), a test with $a = 60000, b = 50000$ gave a
non-bijective map for 16 of 299 even $N$ in 4–600 (every multiple of 38, and 362): those files would
fail the round-trip check on every retry and silently drop out of the sample. At `-O2` the same code
happened to give the correct matrix. The validation draw ($a = 37000, b = 35729$, $ab ≈ 1.32 \times
10^9$) is safe.

Pre-flight action (no code change needed): after the first invocation writes `f.csv.params`, stop and
check `acm_a`, `acm_b`:
- $a \cdot b \le 2{,}147{,}483{,}647$;
- $a \ne 0$, $b \ne 0$ (otherwise every file throws);
- $a \ne b$ (otherwise the Fibonacci path runs; its cache in `acm.cpp:62` is keyed on $n$ only, not on
  the modulus $N$, so values computed for the first file's $N$ are reused for other sizes).

If any fails, delete `f.csv` and `f.csv.params` and start again.

**F4 — `experiment/bbs_params.csv` is not in git and the corpus script does not create it.** On a
fresh EC2 clone, `read_bbs_params` returns an empty list and `.back()` is undefined behaviour. Run
`make experiment-gen-params` first (takes a few minutes for the 1024-bit rows).

**F5 — No manifest builder; SHA-256 is not verified.** `experiment/corpus_manifest.csv` must be built
by hand or by a new script, and it alone enforces the 10 MB cap, the exclusion of the per-subset
documentation PDF, and the type/subset labels. `sha256.cpp` is compiled into `exp-f.o` but never
called: `f.cpp` trusts the manifest's hash and uses it only as the resume key. Compute hashes from the
downloaded files (`sha256sum`) and record each archive's own hash too, since NapierOne is still
evolving. Decide whether "10 MB" means 10,000,000 B or 10 MiB (affects only PDF: 98 vs 99 files).

**F6 — Run record gaps.**
- `f.cpp`, `manifest.*`, `sha256.*` and the tools changes are staged but **not committed**, so
  `git_commit` would point at a commit that does not contain the harness. Commit and push before
  cloning on EC2.
- `git_commit`, `compiler` and `run_date` are overwritten on each resume, so a resumed run records
  only the last invocation.
- The manifest is recorded by path only. Copy it to `experiment/results/` and commit it with
  `f.csv`.
- stderr (the `done:` / `FAILED` / `skip` lines) is not saved. Redirect it, e.g.
  `./run_corpus_experiment.sh 2> experiment/results/f.stderr.log`.
- Instance type, CPU model and kernel are not recorded. Save `lscpu`, `uname -a` and the EC2
  instance type alongside.

**F13 — Output path collided with the old size-sweep results (handled).** The a–f result files under
`experiment/results/` are tracked in git, even though that directory is in `.gitignore`. So a fresh
clone contained the old synthetic `f.csv`, and the corpus run, which resumes into `f.csv`, would have
appended corpus rows under the old header. Handled by renaming the old files to
`f_size_sweep.csv` / `f_size_sweep.stderr.log` (staged, not committed), and by having `f.cpp` refuse
to run if its output CSV has a different header. The refusal happens before the params sidecar is
written. Check that `experiment/results/f.csv` does not exist before the first EC2 invocation.

### Decide before the run

**F7 — Any-channel NPCR has a known non-literature ideal.** Every pixel covers exactly two RT pairs,
so in the current pipeline a pixel is identical between two encryptions only if both $r$ values
repeat: ideal $1 - 2^{-16} = 99.99847\%$, matching the a–e and old-f results (≈ 0.999985). The deprecated
pipeline scatters pair bytes across pixels, so its ideal is $1 - 2^{-24} ≈ 1$. Keep the column for
continuity, but compare against the literature only via `npcr_ch`.

**F8 — Avalanche is measured on PNG file bytes, which biases it low.** PNG/zlib framing (signature,
chunk lengths and types, headers) is identical or near-identical in two cipherimages of the same size.
Checked on the existing 3.2 MB cipherimage pairs in `experiment/dum/`:

| Pair | File-byte avalanche | Decoded-pixel avalanche |
| --- | --- | --- |
| `rt_cipherimage_1/2` | 0.49936 | **0.50003** (SD under H₀: 0.00007) |
| `acm_equal_n_cipherimage_1/2` | 0.49928 | — |

So the unexplained ~10 SD shortfall noted in `new-experiment.md` is a measurement artefact, not a
property of FTIE. The bias grows as files shrink (validation: 0.465 for a 561-byte file). Options:
compute avalanche on decoded pixels (change in `experiment/tools`, which is in scope), or keep the
file-byte version and report the bias. Pixel-level values can be recomputed after the run for the
current pipeline only (§6.3).

**F9 — Which bit to flip.** The code flips bit 0 (`^= 0x01`); `new-experiment.md` M4 says bit 1, as
the old 1→3 change did. Pick one and state it.

**F10 — Deprecated keystream is not frozen.** It is regenerated from `std::random_device` each time
the process starts, so rows before and after a resume use different deprecated keys (the current
pipeline's key is frozen). Also, for files near 10,000,000 B the padded length (e.g. 10,000,086 B at
$N = 2582$) slightly exceeds the 10⁷-byte keystream, which then wraps. Both affect only the deprecated
pipeline; state them or avoid resuming.

**F11 — Timing design.** One measurement per file; it includes file I/O, PNG/zlib and, for the
current pipeline, ~fixed BBS parameter validation, which dominates for small files. The deprecated
helpers take the 10 MB keystream by value three times per call (`tools.cpp:274`, `ftie.cpp:165`,
`rt.cpp:10`), adding a fixed copy cost to every deprecated timing. `high_resolution_clock` is the
system (not steady) clock on libstdc++. Decide whether to repeat (≥ 3, report median) and say what
the time covers.

**F12 — Small files.** `calculate_entropy` has no $p_k > 0$ guard, so a cipherimage with an empty
histogram bin returns NaN (validation: the 561-byte `.gitignore`). Expected empty bins are 1.7 at
1,280 bytes and 0.002 at 3,000 bytes, so this hits only files under about 1.5 KB — e.g. JPG-FROM-WEB
0074 (302 B). Exclude by the §7 floor.

### Notes for the paper (limitations, not blockers)

- **What NPCR/UACI/avalanche measure here.** Because RT adds fresh randomness per byte, the
  differences between cipherimages come from $r$, not from plaintext diffusion. Flipping one
  plaintext bit deterministically changes only the two RT bytes of that position; ACM moves them but
  does not spread the change. So avalanche here measures randomized re-encryption, not the classical
  avalanche/diffusion property. Describe it that way.
- **Pair structure is outside the metric set.** For every plaintext byte,
  $C_{2i} - C_{2i+1} \equiv P'_i - K_i \pmod{256}$, independent of $r$. Histogram-based tests
  (entropy, χ²) and NPCR/UACI cannot see this, and the same $K$ is used for every file. A reviewer
  may ask about tests on byte pairs or correlation; state the scope of the metric set.
- **Randomness source.** Cipherimage uniformity rests on `std::random_device` (on Linux/libstdc++,
  hardware RNG or `/dev/urandom`). Record the platform.
- **Platform dependence.** `calculate_entropy` and `calculate_npcr*` use `uint_fast16_t` /
  `uint_fast32_t` counters: 64-bit on Linux/glibc, but 16/32-bit on macOS, where entropy would
  overflow for large images. Run the experiment on Linux; do not mix macOS numbers in.
- **Representativeness.** It is unknown whether Tiny's 0001–0100 is a representative sample of each
  subset; the NapierOne paper does not say how files were ordered.
- **Earlier results.** a.csv–e.csv and the old size-sweep f.csv were measured on random plaintext and
  should not be used for randomness claims; keep them as a labelled, separate set (§8).

---

## 10. EC2 run procedure

1. **Commit and push** the harness (F6). Note the commit hash.
2. **Instance.** Ubuntu 24.04, x86-64 or Graviton, **not burstable** (avoid t3/t4g: CPU credits
   distort timing). The code is single-threaded, so more vCPUs do not speed it up.
   Memory/disk: see §11.
3. `./setup_vps.sh`; clone the repo at the pushed commit.
4. `make experiment-gen-params` (F4).
5. Download the seven Tiny archives; record each archive's SHA-256 and download date; extract.
6. Build `experiment/corpus_manifest.csv` (F5): apply the size cap, drop the documentation PDFs,
   compute per-file SHA-256. Check the per-type counts against §3.3.
7. **Smoke test**: a manifest of 2–3 small corpus files to `experiment/dum/smoke_f.csv`. Check every
   column is populated and no file FAILED.
8. **Freeze and check parameters**: start the real run, stop it after the params file appears
   (or after the first row), and apply the F3 checks to `f.csv.params`. If a check fails, delete
   `f.csv` and `f.csv.params` and restart.
9. Run under `tmux`, with stderr saved:
   `./run_corpus_experiment.sh 2> experiment/results/f.stderr.log`.
   Monitor with `tail -f experiment/results/f.stderr.log` and `wc -l experiment/results/f.csv`.
   Run nothing else on the instance.
10. **After the run:**
    - rows in `f.csv` (minus header) = rows in the manifest;
    - `grep FAILED experiment/results/f.stderr.log` is empty;
    - no NaN outside the files excluded by the §7 floor;
    - save `lscpu`, `uname -a`, instance type; copy the manifest into `experiment/results/`;
    - commit `f.csv`, `f.csv.params`, the manifest and the environment record.

---

## 11. Resource estimate

- **Time.** The old size sweep (`f_size_sweep.csv`) measured about 16.9 s per MB of plainfile for both pipelines including
  metrics. For ≈ 672 MB that is about **3.2 h**. A sizing run of one 10,000,000-byte random file
  through the full `f.cpp` procedure (all metrics, macOS/Apple Silicon, `-O0`) took **134 s**
  (≈ 13.4 s/MB): enc/dec 15.4/8.2 s deprecated, 19.2/17.0 s current. EC2 will differ; the smoke
  test (§10 step 7) gives the real rate.
- **Memory.** Peak resident memory for that 10 MB file was **432 MB** (macOS). A 2 GB instance is
  enough; 4 GB leaves margin.
- **Disk.** The four scratch files per corpus file total about 6× its size (cipherimages are ≈ 2.03×
  the plainfile), so ≈ **4 GB** for the corpus, plus the downloaded archives and extracted files.

---

## 12. Requirements traceability (`new-experiment.md`)

| Req. | Status in code |
| --- | --- |
| D1 Loader | Done (`manifest.cpp`, `f.cpp`) |
| D2–D6 Coverage, exclusions, tier, selection, independence | Enforced only by the manifest, which does not exist yet (F5) |
| D7 Provenance | Fields in manifest; SHA-256 not verified; tier/date not in `f.csv` |
| D8 Attribution | Paper-side |
| M1 UACI | Done; critical range has units bug (F1) |
| M2 χ² | Done, pooled R+G+B (stated in `tools.h`, not in output) |
| M3 Per-image thresholds | Done; NPCR/UACI variance assumption wrong (F2) |
| M4 Pair definition | Done; flipped bit differs from spec (F9) |
| M5 Padding in sample | Answered: all $3N^2$ bytes are RT output (§2.2) |
| M6 Existing metrics | Unchanged |
| M7 Plaintext baseline | Done |
| O1–O3 Row per file, columns, both pipelines | Done |
| O4 Per-type summary | Not implemented |
| O5 Run record | Partial (F6) |
| O6 Resumable | Done, by SHA-256; deprecated keystream not frozen (F10) |
| O7 No overwrite of old results | a–e untouched; new `f.cpp` refuses to append to the old size-sweep `f.csv` (different header), see F13 |
| O8 Timing from corpus | Done |

---

## 13. References

- Davies, S. R., Macfarlane, R., Buchanan, W. J. "NapierOne: A modern mixed file data set
  alternative to Govdocs1." *Forensic Science International: Digital Investigation* 40 (2022) 301330.
  doi:10.1016/j.fsidi.2021.301330
- Wu, Y., Noonan, J. P., Agaian, S. "NPCR and UACI Randomness Tests for Image Encryption."
  *Cyber Journals: JSAT* (2011). — source of $\mu_N = F/(F+1)$, $\mu_U = (F+2)/(3F+3)$ and their
  variances. Critical-value tables not yet checked against this implementation.
- Wu, Y., Noonan, J. P., Agaian, S. "Shannon Entropy based Randomness Measurement and Test for Image
  Encryption." arXiv:1103.5520.
- NIST/SEMATECH e-Handbook of Statistical Methods, §7.2.4 (sample size for proportions).
- Blum, L., Blum, M., Shub, M. "A Simple Unpredictable Pseudo-Random Number Generator." *SIAM J.
  Computing* 15(2) (1986) 364–383.
