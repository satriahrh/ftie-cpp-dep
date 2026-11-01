# Experiment results

Results of the 2026-10-16 run of experiments a–e and corpus f (EC2 c7i.large, clean commit
`cb6c1cf`, results committed in `25c9e8a`), and their analysis. What each experiment measures, the
metrics and thresholds, and the results with their reading are in
[`experiment/METHODOLOGY.md`](../METHODOLOGY.md): §5 (a–e), §6 (f), §7 (file layout and columns),
§8.4 (results).

Every experiment runs two pipelines on the same input: the **proposed** pipeline (BBS keystream, RT,
PNG packing, pixel-level ACM) and the **previous** pipeline (literal keystream, RT, byte-level ACM,
then PNG packing). Their result columns end in `_proposed` and `_previous`.

## Raw results

| File | Content |
| --- | --- |
| `a.csv` … `e.csv` | One row per sweep value (30 each): `prime_bits, acm_a, acm_b, acm_n`, then per pipeline `npcr, npcr_ch, uaci, entropy, chi2, avalanche, enc_s, dec_s`; e adds `modulus` |
| `f.csv` | One row per corpus file (692): file identity, cipherimage size, plaintext baseline, the same metrics per pipeline, and per-file critical values |
| `f.csv.params` | Frozen key of f: BBS row, ACM $a, b, n$, keystream length and SHA-256, manifest path, commit, compiler, date |
| `f.archives.sha256` | SHA-256 of the seven NapierOne Tiny archives |
| `env.txt` | Machine and code state, one block per run-script invocation |
| `run.log`, `build.log`, `gen-params.log`, `*.stderr.log` | Logs of the run (a–e stderr logs are empty; `f.stderr.log` has one `done:` line per file) |

The corpus file list with SHA-256 is `experiment/corpus_manifest.csv`, and the BBS keys of every
experiment are in `experiment/bbs_params.csv`. `f.csv.keystream` (the previous pipeline's 10 MB
keystream for f) is gitignored.

| Experiment | Swept | Fixed |
| --- | --- | --- |
| a | ACM iterations $n$ = 10…300, $a = b$ | random $a = b$, 1024-bit BBS |
| b | ACM iterations $n$ = 10…300, $a \ne b$ | random $a \ne b$, 1024-bit BBS |
| c | ACM entry $a = b$ = 1…63,337 | $n = 50$, 1024-bit BBS |
| d | ACM entry $a$ = 1…63,337, $b = a + 1$ | $n = 50$, 1024-bit BBS |
| e | BBS prime size 16…1024 bits | random $a, b$, $n = 50$ |
| f | 692 NapierOne files of 7 types, natural sizes up to 10,000,000 B | $a = 40{,}820$, $b = 61{,}719$, $n = 50$, 1024-bit BBS |

a–e encrypt one 3,210,000-byte random plaintext per experiment, so they are evidence about cost
only (METHODOLOGY §5.3, §5.4). f is the evidence about quality on real files.

## Analysis (`analysis/`)

Produced by `experiment/analyze.py` (`make experiment-analyze`, after
`python3 -m venv .venv && .venv/bin/pip install -r experiment/requirements.txt`). It only reads the
files above.

| File | Content |
| --- | --- |
| `report.txt` | Everything the script prints: integrity check, decision tables, size-free checks, descriptive tables, timing fits, a–e checks |
| `f_decisions.csv` | Per type, test (χ², entropy, NPCR_ch, UACI) and pipeline: n, failures, cutoff, accept/reject; plus a pooled row per test |
| `f_uniformity.csv` | KS tests of χ² p-values and z-scores per type, Kruskal–Wallis across types, Spearman against plaintext entropy |
| `f_summary.csv` | Per type and pipeline: n, mean, SD, min, max of every metric |
| `f_per_file.csv` | Per file: χ² p-value, NPCR_ch / UACI / avalanche z-scores, pass/fail per test, included flag |
| `excluded.csv` | Files excluded by the 1,280-byte rule (one: `0074-jpg.jpg`, 302 B) |
| `timing_fits.csv` | Linear (and, for a–e, quadratic) timing fits for f and a–e |
| `ae_quality.csv` | a–e quality checks against the thresholds at $N = 1464$ |
| `f_timing.png` | Encrypt and decrypt time vs plaintext size, both pipelines |
| `f_entropy.png` | Entropy shortfall $8 - H$ vs size, raw and divided by its expected value |
| `f_chi2_pvalues.png` | χ² p-value histograms per type and pipeline |
| `f_statistics_by_type.png` | Per-file χ² p-values and NPCR_ch / UACI / avalanche z-scores by type |
| `ae_timing.png` | a–e time vs the swept parameter |

## Older files

`f_size_sweep.csv` / `f_size_sweep.stderr.log` are the output of an older experiment f (plaintext
0.5–10 MB of random bytes). The corpus experiment replaced it; it is not part of the results.
