#!/usr/bin/env python3
"""Analysis of the a-e and corpus-f results (experiment/METHODOLOGY.md section 8).

Reads experiment/results/{a..f}.csv and experiment/corpus_manifest.csv and writes
tables, figures and a text report to experiment/results/analysis/. Run from the
repo root:

    python3 -m venv .venv && .venv/bin/pip install -r experiment/requirements.txt
    .venv/bin/python experiment/analyze.py

Hypotheses (H1-H8) and decisions are those of the analysis plan; the per-file
critical values are taken from f.csv, not recomputed.
"""
import functools
import math
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
import pandas as pd  # noqa: E402
from scipy import stats  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "experiment" / "results"
OUT = RESULTS / "analysis"
MANIFEST = ROOT / "experiment" / "corpus_manifest.csv"
BBS_PARAMS = ROOT / "experiment" / "bbs_params.csv"

ALPHA = 0.05
TYPE_FALSE_REJECT = 0.01  # per type and metric (section 8.2)
CHI2_DF = 255
CHI2_CRIT = 293.25  # chi2(0.95, 255)
MIN_SAMPLE_BYTES = 1280  # 5 expected per bin (section 4.2)
OUTLIER_REL = 0.10  # timing residual flagged only if also > 10% of the fitted time
CSV_FLOAT = "%.10g"

# METHODOLOGY.md section 4.8: the null of a re-encryption pair depends on the pipeline. Previous:
# both encryptions share K, so the two bytes of an RT pair move together (1.5*W*H samples, pair
# variance). Proposed: chained seeds give each encryption its own K, so the 3*W*H channel bytes
# are independent (single-byte variance).
NPCR_MU = 255 / 256
UACI_MU = 100 * 257 / 768
UACI_VAR_PAIR = 0.0391966
UACI_VAR_BYTE = 0.0559930
SHARED_KEYSTREAM = {"proposed": False, "previous": True}
AE_N = 1464  # cipherimage side in a-e (3,210,000-byte plainfile)

# Proposed pipeline's BBS seeds per row (METHODOLOGY.md section 3): Enc(P1)->C1, second Enc(P1),
# Enc(P2), and the running seed after them. ~617-digit integers, so read as strings.
SEED_COLS = [f"seed_{k}_proposed" for k in ("c1", "c2", "c3", "next")]
SEED_DTYPES = {col: str for col in SEED_COLS}

# Pipelines, as named in the result columns (<metric>_proposed, <metric>_previous) and every
# output: proposed = ftie::encrypt, previous = ftie::previous::encrypt.
PIPELINES = ("proposed", "previous")
TYPE_ORDER = ["DOCX", "XLSX", "PPTX", "PDF", "MP3", "MP4", "JPG"]
TESTS = ["chi2", "entropy", "npcr_ch", "uaci"]  # per-file tests with a pass rule

# Light chart surface and ink; series in fixed categorical order (dataviz palette)
SURFACE, INK, INK_MUTED, GRID = "#fcfcfb", "#1f1f1e", "#6b6a64", "#e4e3de"
COLOR = {"proposed": "#2a78d6", "previous": "#eb6834", "plain": "#1baf7a"}

report_lines = []


def say(text=""):
    print(text)
    report_lines.append(text)


def table(df):
    say(df.to_string(index=False, float_format=lambda v: f"{v:.6g}"))
    say()


def write_csv(df, name):
    df.to_csv(OUT / name, index=False, float_format=CSV_FLOAT)


# --- avalanche null distribution -------------------------------------------------------------

def avalanche_pair_variance():
    """Exact variance of the bits flipped in one RT pair between Enc(P1) and Enc(P2).

    Both bytes of a pair shift by the same delta = r' - r, so the flip count is
    f(c, delta) + f(c', delta) with c, c' independent uniform and delta shared:
    Var = 2 Var(f) + 2 Var_delta(E_c f). The same enumeration reproduces the UACI
    pair variance of section 4.8 (0.0391966).
    """
    v = np.arange(256)
    popcount = np.array([bin(i).count("1") for i in range(256)])
    flips = popcount[v[:, None] ^ ((v[:, None] + v[None, :]) & 255)]  # [c, delta]
    return 2 * flips.var() + 2 * flips.mean(axis=0).var()


AVALANCHE_VAR_PAIR = avalanche_pair_variance()  # bits^2 per pair of 16 bits


def null_model(pipe, side):
    """(samples, UACI variance, avalanche SD) of one pipeline's null at cipherimage side N."""
    if SHARED_KEYSTREAM[pipe]:
        pairs = 1.5 * side * side
        return pairs, UACI_VAR_PAIR, np.sqrt(AVALANCHE_VAR_PAIR / 256 / pairs)
    return 3 * side * side, UACI_VAR_BYTE, 0.5 / np.sqrt(24 * side * side)


def npcr_critical(pipe, side):
    samples, _, _ = null_model(pipe, side)
    return NPCR_MU - 1.645 * np.sqrt(NPCR_MU * (1 - NPCR_MU) / samples)


def uaci_half_range(pipe, side):
    samples, var, _ = null_model(pipe, side)
    return 100 * 1.96 * np.sqrt(var / samples)


def z_scores(df, pipe, side):
    """Size-free statistics for one pipeline: chi2 p-value and z-scores."""
    samples, var, aval_sd = null_model(pipe, side)
    out = pd.DataFrame(index=df.index)
    out["chi2_p"] = stats.chi2.sf(df[f"chi2_{pipe}"], CHI2_DF)
    out["npcr_ch_z"] = (df[f"npcr_ch_{pipe}"] - NPCR_MU) / np.sqrt(NPCR_MU * (1 - NPCR_MU) / samples)
    out["uaci_z"] = (df[f"uaci_{pipe}"] - UACI_MU) / (100 * np.sqrt(var / samples))
    out["avalanche_z"] = (df[f"avalanche_{pipe}"] - 0.5) / aval_sd
    return out


@functools.lru_cache(maxsize=None)
def acm_period(a, b, side):
    """Period of the proposed pipeline's pixel map A = [[1, a], [b, 1 + ab]] mod N."""
    m00, m01, m10, m11 = 1, a % side, b % side, (1 + a * b) % side
    x00, x01, x10, x11 = m00, m01, m10, m11
    k = 1
    while (x00, x01, x10, x11) != (1, 0, 0, 1):
        x00, x01, x10, x11 = ((x00 * m00 + x01 * m10) % side, (x00 * m01 + x01 * m11) % side,
                              (x10 * m00 + x11 * m10) % side, (x10 * m01 + x11 * m11) % side)
        k += 1
    return k


def acm_identity(a, b, n, side):
    """(period, whether A^n mod N is the identity) for each row."""
    period = pd.Series([acm_period(int(x), int(y), int(z)) for x, y, z in zip(a, b, side)], index=a.index)
    return period, (n % period == 0)


def read_params(path):
    return dict(line.strip().split("=", 1) for line in path.read_text().splitlines() if "=" in line)


def check_seed_chain(df, name, start_seed, chained):
    """Issues with a CSV's seed chain. start_seed: the s each chain starts from (one per row
    when every row is its own key). chained: each row continues from the previous row's next seed."""
    issues = []
    seeds = df[SEED_COLS]
    if seeds.isna().any().any():
        return [f"{name}: missing seed values"]
    if (seeds.nunique(axis=1) != len(SEED_COLS)).any():
        issues.append(f"{name}: {(seeds.nunique(axis=1) != len(SEED_COLS)).sum()} rows reuse a seed")
    c1 = df["seed_c1_proposed"]
    if chained:
        if c1.iloc[0] != start_seed:
            issues.append(f"{name}: first row's seed_c1 is not the key's s")
        broken = (c1.iloc[1:].values != df["seed_next_proposed"].iloc[:-1].values).sum()
        if broken:
            issues.append(f"{name}: {broken} rows do not continue from the previous row's next seed")
    elif (c1 != start_seed).any():
        issues.append(f"{name}: {(c1 != start_seed).sum()} rows do not start from their key's s")
    return issues


def type_cutoff(n):
    """Largest failure count at which a type is accepted (binomial 99th percentile)."""
    return int(stats.binom.ppf(1 - TYPE_FALSE_REJECT, n, ALPHA))


def side_of(size_bytes):
    """Cipherimage side N from the padding formula (METHODOLOGY.md section 2.2)."""
    n = math.ceil(math.sqrt(math.ceil(2 * (size_bytes + 4) / 3)))
    return n + (n % 2)


# --- step 0: integrity and exclusions --------------------------------------------------------

def check_integrity(f):
    say("== 0. Data integrity ==")
    issues = []
    man = pd.read_csv(MANIFEST)
    if f["sha256"].duplicated().any():
        issues.append(f"{f['sha256'].duplicated().sum()} duplicate sha256 in f.csv")
    joined = f.merge(man, on="sha256", how="left", suffixes=("", "_man"), indicator=True)
    if (joined["_merge"] != "both").any():
        issues.append(f"{(joined['_merge'] != 'both').sum()} f.csv rows not in the manifest")
    missing = set(man["sha256"]) - set(f["sha256"])
    if missing:
        issues.append(f"{len(missing)} manifest files missing from f.csv")
    for col in ["type", "subset", "file_name", "size_bytes"]:
        bad = (joined[col] != joined[f"{col}_man"]).sum()
        if bad:
            issues.append(f"{bad} rows where {col} differs from the manifest")

    if (f["width"] != f["height"]).any() or (f["width"] % 2).any():
        issues.append("cipherimage not square or side not even")
    expected = f["size_bytes"].map(side_of)
    if (expected != f["width"]).any():
        issues.append(f"{(expected != f['width']).sum()} rows where width != padding formula")

    for pipe in PIPELINES:
        if not np.allclose(npcr_critical(pipe, f["width"]), f[f"npcr_crit_{pipe}"], rtol=0, atol=1e-8):
            issues.append(f"npcr_crit_{pipe} column differs from the section 4.8 formula")
        half = uaci_half_range(pipe, f["width"])
        if not (np.allclose(UACI_MU - half, f[f"uaci_crit_low_{pipe}"], atol=1e-6)
                and np.allclose(UACI_MU + half, f[f"uaci_crit_high_{pipe}"], atol=1e-6)):
            issues.append(f"uaci_crit_*_{pipe} columns differ from the section 4.8 formula")
    entropy_crit = 8 - CHI2_CRIT / (2 * 3 * f["width"] * f["height"] * math.log(2))
    if not np.allclose(entropy_crit, f["entropy_crit"], atol=1e-8):
        issues.append("entropy_crit column differs from the section 4.8 formula")

    log = (RESULTS / "f.stderr.log").read_text().splitlines()
    done = [l for l in log if l.startswith("done:")]
    other = [l for l in log if l.strip() and not l.startswith(("done:", "==="))]
    if len(done) != len(f):
        issues.append(f"f.stderr.log has {len(done)} done lines for {len(f)} rows")
    if other:
        issues.append(f"f.stderr.log has {len(other)} unexpected lines, e.g. {other[0]!r}")
    issues += check_seed_chain(f, "f.csv", read_params(RESULTS / "f.csv.params")["s"], chained=True)

    say(f"f.csv rows: {len(f)}; manifest files: {len(man)}; done lines: {len(done)}")
    say("per type: " + ", ".join(f"{t} {n}" for t, n in f["type"].value_counts()[TYPE_ORDER].items()))
    if issues:
        say("ISSUES:")
        for issue in issues:
            say(f"  - {issue}")
    else:
        say("No issues: every row matches the manifest, sides match the padding formula, "
            "critical-value columns match section 4.8, log has only done lines, "
            "seed chain starts at s and is unbroken.")
    say()
    return issues


def exclusions(f):
    plain_small = f["size_bytes"] < MIN_SAMPLE_BYTES
    cipher_small = 3 * f["width"] * f["height"] < MIN_SAMPLE_BYTES
    rows = []
    for idx in f.index[plain_small | cipher_small]:
        r = f.loc[idx]
        rows.append({
            "type": r["type"], "file_name": r["file_name"], "size_bytes": r["size_bytes"],
            "cipher_bytes": 3 * r["width"] * r["height"],
            "excluded_from": "; ".join(
                (["plainfile entropy/chi2"] if plain_small[idx] else [])
                + (["all cipherimage tests"] if cipher_small[idx] else [])),
        })
    excl = pd.DataFrame(rows, columns=["type", "file_name", "size_bytes", "cipher_bytes", "excluded_from"])
    say("Excluded (sample under 1,280 bytes; cipher exclusion also covers NPCR_ch/UACI, whose normal "
        "approximation needs the same size):")
    table(excl) if len(excl) else say("none\n")
    write_csv(excl, "excluded.csv")
    return ~cipher_small, ~plain_small


# --- step 1: corpus f ------------------------------------------------------------------------

def per_file_tests(f, pipe):
    return pd.DataFrame({
        "chi2": f[f"chi2_{pipe}"] > CHI2_CRIT,
        "entropy": f[f"entropy_{pipe}"] < f["entropy_crit"],
        "npcr_ch": f[f"npcr_ch_{pipe}"] < f[f"npcr_crit_{pipe}"],
        "uaci": (f[f"uaci_{pipe}"] < f[f"uaci_crit_low_{pipe}"]) | (f[f"uaci_{pipe}"] > f[f"uaci_crit_high_{pipe}"]),
    }, index=f.index)  # True = file fails


def decisions(f, keep, fails):
    say("== 1.1-1.2 Per-type decisions (H1 chi2/entropy, H2 npcr_ch/uaci; H5 = proposed vs previous) ==")
    rows = []
    for pipe in PIPELINES:
        for test in TESTS:
            for t in TYPE_ORDER + ["ALL"]:
                m = keep & ((f["type"] == t) if t != "ALL" else True)
                n, k = int(m.sum()), int(fails[pipe].loc[m, test].sum())
                row = {"pipeline": pipe, "test": test, "type": t, "n": n, "failures": k,
                       "fail_rate": k / n}
                if t == "ALL":
                    row["accept_if_at_most"] = np.nan
                    row["binom_p_two_sided"] = stats.binomtest(k, n, ALPHA).pvalue
                    row["decision"] = "pooled"
                else:
                    cut = type_cutoff(n)
                    row["accept_if_at_most"] = cut
                    row["binom_p_two_sided"] = np.nan
                    row["decision"] = "accept" if k <= cut else "REJECT"
                rows.append(row)
    dec = pd.DataFrame(rows)
    write_csv(dec, "f_decisions.csv")

    wide = dec[dec["type"] != "ALL"].pivot_table(
        index=["pipeline", "test"], columns="type", values="failures", aggfunc="first")[TYPE_ORDER]
    pooled = dec[dec["type"] == "ALL"].set_index(["pipeline", "test"])[["n", "failures", "binom_p_two_sided"]]
    say("Failures per type (cutoffs: " + ", ".join(
        f"n={n} -> {type_cutoff(n)}" for n in sorted(set(dec.loc[dec['type'] != 'ALL', 'n']))) + "):")
    table(wide.join(pooled).reset_index())
    rejected = dec[dec["decision"] == "REJECT"]
    n_dec = int((dec["type"] != "ALL").sum())
    say(f"{len(rejected)} of {n_dec} type-level decisions rejected "
        f"(about {n_dec * TYPE_FALSE_REJECT:.2f} false rejects expected at <= 1% each).")
    if len(rejected):
        table(rejected[["pipeline", "test", "type", "n", "failures", "accept_if_at_most"]])
    say()
    return dec


def uniformity(f, keep, zs):
    say("== 1.3 Size-free checks (H1, H4): chi2 p ~ U(0,1), z ~ N(0,1) under H0 ==")
    rows = []
    for pipe in PIPELINES:
        z = zs[pipe][keep]
        for col, dist in [("chi2_p", "uniform"), ("npcr_ch_z", "norm"), ("uaci_z", "norm"),
                          ("avalanche_z", "norm")]:
            for t in TYPE_ORDER + ["ALL"]:
                x = z[col] if t == "ALL" else z.loc[f.loc[keep, "type"] == t, col]
                ks = stats.kstest(x, dist)
                rows.append({"pipeline": pipe, "statistic": col, "test": "KS", "group": t, "n": len(x),
                             "mean": x.mean(), "sd": x.std(), "stat": ks.statistic, "p": ks.pvalue})
            groups = [z.loc[f.loc[keep, "type"] == t, col] for t in TYPE_ORDER]
            kw = stats.kruskal(*groups)
            rows.append({"pipeline": pipe, "statistic": col, "test": "Kruskal-Wallis across types",
                         "group": "ALL", "n": len(z), "stat": kw.statistic, "p": kw.pvalue})
            plain_ok = f.loc[keep, "size_bytes"] >= MIN_SAMPLE_BYTES
            rho = stats.spearmanr(f.loc[keep, "entropy_plain"][plain_ok], z[col][plain_ok])
            rows.append({"pipeline": pipe, "statistic": col, "test": "Spearman vs entropy_plain",
                         "group": "ALL", "n": int(plain_ok.sum()), "stat": rho.correlation,
                         "p": rho.pvalue})
    uni = pd.DataFrame(rows)
    write_csv(uni, "f_uniformity.csv")

    ks = uni[uni["test"] == "KS"]
    say("KS p-values per type (small p = departs from the null distribution):")
    table(ks.pivot_table(index=["pipeline", "statistic"], columns="group", values="p",
                         aggfunc="first")[TYPE_ORDER + ["ALL"]].reset_index())
    say("Pooled mean/SD of each statistic (null: chi2_p 0.5/0.289, z 0/1):")
    table(ks[ks["group"] == "ALL"][["pipeline", "statistic", "n", "mean", "sd"]])
    say("Type effect and plainfile-entropy effect:")
    table(uni[uni["test"] != "KS"][["pipeline", "statistic", "test", "n", "stat", "p"]])
    n_tests = len(uni)
    say(f"{n_tests} tests in this section; at alpha 0.05 about {n_tests * ALPHA:.1f} small p-values "
        "are expected by chance.\n")
    return uni


def chi2_passes(f, keep, plain_keep):
    """Files with chi2 <= CHI2_CRIT, as (passes, n) for the plainfile and each pipeline."""
    out = {"plain": (int((f.loc[plain_keep, "chi2_plain"] <= CHI2_CRIT).sum()), int(plain_keep.sum()))}
    for pipe in PIPELINES:
        out[pipe] = (int((f.loc[keep, f"chi2_{pipe}"] <= CHI2_CRIT).sum()), int(keep.sum()))
    return out


def describe(f, keep, plain_keep, zs):
    say("== 1.4-1.7 Descriptive statistics (O4), plainfile vs cipher (H4), avalanche (H3) ==")
    metrics = ["npcr", "npcr_ch", "uaci", "entropy", "chi2", "avalanche", "enc", "dec"]
    rows = []
    for t in TYPE_ORDER + ["ALL"]:
        m = keep & ((f["type"] == t) if t != "ALL" else True)
        sub = f[m]
        for col, pipe in [("size_bytes", "-"), ("entropy_plain", "plain"), ("chi2_plain", "plain")]:
            x = sub[col] if col == "size_bytes" else sub.loc[sub["size_bytes"] >= MIN_SAMPLE_BYTES, col]
            rows.append({"type": t, "pipeline": pipe, "metric": col, "n": len(x), "mean": x.mean(),
                         "sd": x.std(), "min": x.min(), "max": x.max()})
        for pipe in PIPELINES:
            for met in metrics:
                col = f"{met}_{pipe}_s" if met in ("enc", "dec") else f"{met}_{pipe}"
                x = sub[col]
                rows.append({"type": t, "pipeline": pipe, "metric": met, "n": len(x), "mean": x.mean(),
                             "sd": x.std(), "min": x.min(), "max": x.max()})
            short = 8 - sub[f"entropy_{pipe}"]
            rows.append({"type": t, "pipeline": pipe, "metric": "entropy_shortfall", "n": len(short),
                         "mean": short.mean(), "sd": short.std(), "min": short.min(), "max": short.max()})
            az = zs[pipe].loc[m, "avalanche_z"]
            rows.append({"type": t, "pipeline": pipe, "metric": "avalanche_z", "n": len(az),
                         "mean": az.mean(), "sd": az.std(), "min": az.min(), "max": az.max(),
                         "abs_z_over_1.96": int((az.abs() > 1.96).sum())})
    desc = pd.DataFrame(rows)
    write_csv(desc, "f_summary.csv")

    def wide(metric, pipe, stat="mean"):
        d = desc[(desc["metric"] == metric) & (desc["pipeline"] == pipe)].set_index("type")[stat]
        return d[TYPE_ORDER + ["ALL"]]

    say("Plainfile vs cipherimage, per type (mean):")
    table(pd.DataFrame({
        "size_KB": wide("size_bytes", "-") / 1000,
        "entropy_plain": wide("entropy_plain", "plain"),
        "entropy_proposed": wide("entropy", "proposed"),
        "entropy_previous": wide("entropy", "previous"),
        "chi2_plain": wide("chi2_plain", "plain"),
        "chi2_proposed": wide("chi2", "proposed"),
        "chi2_previous": wide("chi2", "previous"),
    }).reset_index())
    say(f"Files passing chi2 <= {CHI2_CRIT} (alpha 0.05): " + ", ".join(
        f"{name} {k} of {n}" for name, (k, n) in chi2_passes(f, keep, plain_keep).items()))
    for col in ["chi2_plain"] + [f"chi2_{pipe}" for pipe in PIPELINES]:
        x = f.loc[plain_keep if col == "chi2_plain" else keep, col]
        say(f"  {col}: min {x.min():.6g}, max {x.max():.6g}")
    say()
    say("Differential metrics per type (mean; NPCR_ch/avalanche as fraction, UACI in %):")
    table(pd.DataFrame({
        "npcr_ch_proposed": wide("npcr_ch", "proposed"), "npcr_ch_previous": wide("npcr_ch", "previous"),
        "uaci_proposed": wide("uaci", "proposed"), "uaci_previous": wide("uaci", "previous"),
        "aval_proposed": wide("avalanche", "proposed"), "aval_previous": wide("avalanche", "previous"),
    }).reset_index())
    say(f"Avalanche (H3, no pass rule): null SD from 24N^2 independent bits (proposed) and from the "
        f"RT pair variance {AVALANCHE_VAR_PAIR:.6f} bits^2 (previous; naive 4.0). z-scores per type:")
    av = desc[desc["metric"] == "avalanche_z"]
    table(av.pivot_table(index="type", columns="pipeline",
                         values=["mean", "sd", "abs_z_over_1.96"], aggfunc="first")
          .reindex(TYPE_ORDER + ["ALL"]).reset_index())

    say("Any-channel NPCR model check (section 4.3):")
    for pipe, ideal in [("proposed", 1 - 2 ** -24), ("previous", 1 - 2 ** -24)]:
        x = f.loc[keep, f"npcr_{pipe}"]
        say(f"  {pipe}: mean {x.mean():.8f}, ideal {ideal:.8f}, min {x.min():.8f}, max {x.max():.8f}")
    say()
    return desc


def timing_fits(f):
    say("== 1.8 Timing vs size (H6) ==")
    rows, outliers = [], []
    mb = f["size_bytes"] / 1e6
    for pipe in PIPELINES:
        for op in ["enc", "dec"]:
            y = f[f"{op}_{pipe}_s"]
            fit = stats.linregress(mb, y)
            fitted = fit.intercept + fit.slope * mb
            resid = y - fitted
            robust_sd = 1.4826 * np.median(np.abs(resid - np.median(resid)))
            # both far out statistically and large in practice (the proposed pipeline's fit is
            # tight enough that 4 robust SD is a few hundredths of a second)
            flag = (np.abs(resid) > 4 * robust_sd) & (np.abs(resid) > OUTLIER_REL * fitted)
            for idx in f.index[flag]:
                outliers.append(f"{pipe} {op}: {f.at[idx, 'file_name']} "
                                f"({f.at[idx, 'size_bytes'] / 1e6:.2f} MB, {y[idx]:.2f} s, "
                                f"fitted {fitted[idx]:.2f} s)")
            rows.append({"experiment": "f", "pipeline": pipe, "op": op, "x": "size_MB", "n": len(y),
                         "slope": fit.slope, "slope_se": fit.stderr, "intercept": fit.intercept,
                         "intercept_se": fit.intercept_stderr, "r2": fit.rvalue ** 2,
                         "outliers": int(flag.sum())})
    fits = pd.DataFrame(rows)
    table(fits[["pipeline", "op", "n", "slope", "slope_se", "intercept", "intercept_se", "r2",
                "outliers"]])
    small = f.nsmallest(3, "size_bytes")
    say("Fixed cost, three smallest files (s): " + "; ".join(
        f"{r.file_name} {r.size_bytes} B: enc proposed {r.enc_proposed_s:.4f} / previous {r.enc_previous_s:.4f}"
        for r in small.itertuples()))
    if outliers:
        say(f"Residual outliers (|residual| > 4 robust SD and > {OUTLIER_REL:.0%} of fitted):")
        for line in outliers:
            say(f"  {line}")
    say()
    return fits


# --- step 2: a-e -----------------------------------------------------------------------------

AE = {  # experiment -> (swept column, hypothesis)
    "a": ("acm_n", "H8a: previous grows linearly with n, proposed flat"),
    "b": ("acm_n", "H8a: previous grows linearly with n, proposed flat"),
    "c": ("acm_a", "H8b: flat in a"),
    "d": ("acm_a", "H8b: flat in a"),
    "e": ("prime_bits", "H8c: proposed grows with modulus size, previous flat"),
}


def analyse_ae():
    say("== 2. Experiments a-e ==")
    entropy_crit = 8 - CHI2_CRIT / (2 * 3 * AE_N * AE_N * math.log(2))
    npcr_crit = {pipe: npcr_critical(pipe, AE_N) for pipe in PIPELINES}
    half = {pipe: uaci_half_range(pipe, AE_N) for pipe in PIPELINES}
    for pipe in PIPELINES:
        say(f"Thresholds at N={AE_N}, {pipe}: npcr_ch >= {npcr_crit[pipe]:.6f}, uaci in "
            f"[{UACI_MU - half[pipe]:.4f}, {UACI_MU + half[pipe]:.4f}], entropy >= {entropy_crit:.7f}, "
            f"chi2 <= {CHI2_CRIT}")
    bbs_s = pd.read_csv(BBS_PARAMS, dtype={"s": str}).set_index("bits")["s"]

    fit_rows, quality_rows, data, issues = [], [], {}, []
    for letter, (xcol, hyp) in AE.items():
        d = pd.read_csv(RESULTS / f"{letter}.csv", dtype=SEED_DTYPES)
        data[letter] = d
        # every row of e is its own key; a-d run one chain from the last bbs_params row's s
        if letter == "e":
            issues += check_seed_chain(d, "e.csv", d["prime_bits"].map(bbs_s), chained=False)
        else:
            issues += check_seed_chain(d, f"{letter}.csv", bbs_s[d["prime_bits"].iloc[0]], chained=True)
        _, identity = acm_identity(d["acm_a"], d["acm_b"], d["acm_n"], pd.Series(AE_N, index=d.index))
        for pipe in PIPELINES:
            for op in ["enc", "dec"]:
                y = d[f"{op}_{pipe}_s"]
                fit = stats.linregress(d[xcol], y)
                span = d[xcol].max() - d[xcol].min()
                quad = np.polyval(np.polyfit(d[xcol], y, 2), d[xcol])
                r2_quad = 1 - ((y - quad) ** 2).sum() / ((y - y.mean()) ** 2).sum()
                fit_rows.append({"experiment": letter, "pipeline": pipe, "op": op, "x": xcol, "n": len(y),
                                 "slope": fit.slope, "slope_se": fit.stderr, "intercept": fit.intercept,
                                 "r2": fit.rvalue ** 2, "r2_quadratic": r2_quad, "p_slope": fit.pvalue,
                                 "min_s": y.min(), "max_s": y.max(),
                                 "fitted_change_pct": 100 * fit.slope * span / y.mean()})
            fails = {
                "chi2": int((d[f"chi2_{pipe}"] > CHI2_CRIT).sum()),
                "entropy": int((d[f"entropy_{pipe}"] < entropy_crit).sum()),
                "npcr_ch": int((d[f"npcr_ch_{pipe}"] < npcr_crit[pipe]).sum()),
                "uaci": int(((d[f"uaci_{pipe}"] < UACI_MU - half[pipe])
                             | (d[f"uaci_{pipe}"] > UACI_MU + half[pipe])).sum()),
            }
            zs = z_scores(d, pipe, AE_N)
            rho = {met: stats.spearmanr(d[xcol], d[f"{met}_{pipe}"]) for met in
                   ["npcr_ch", "uaci", "chi2", "avalanche"]}
            quality_rows.append({
                "experiment": letter, "pipeline": pipe, "n": len(d),
                **{f"fail_{k}": v for k, v in fails.items()},
                "npcr_any_mean": d[f"npcr_{pipe}"].mean(),
                "npcr_ch_mean": d[f"npcr_ch_{pipe}"].mean(), "uaci_mean": d[f"uaci_{pipe}"].mean(),
                "entropy_mean": d[f"entropy_{pipe}"].mean(), "chi2_mean": d[f"chi2_{pipe}"].mean(),
                "avalanche_mean": d[f"avalanche_{pipe}"].mean(),
                "avalanche_z_mean": zs["avalanche_z"].mean(), "avalanche_z_sd": zs["avalanche_z"].std(),
                "chi2_p_ks": stats.kstest(zs["chi2_p"], "uniform").pvalue,
                **{f"spearman_p_{met}_vs_x": r.pvalue for met, r in rho.items()},
                # the pixel map is the proposed pipeline's only; previous permutes bytes on another grid
                "acm_identity_rows": int(identity.sum()) if pipe == "proposed" else np.nan,
            })

    fits = pd.DataFrame(fit_rows)
    quality = pd.DataFrame(quality_rows)
    write_csv(quality, "ae_quality.csv")
    say("\nTiming fits (H8); fitted_change_pct = fitted change over the sweep / mean time:")
    for letter, (xcol, hyp) in AE.items():
        say(f"{letter} ({xcol}) - {hyp}")
        sub = fits[fits["experiment"] == letter]
        table(sub[["pipeline", "op", "slope", "slope_se", "r2", "r2_quadratic", "p_slope", "min_s", "max_s",
                   "fitted_change_pct"]])
    say("Quality (H7): failures out of 30 rows at alpha 0.05 (about 1.5 expected per test), "
        "and Spearman p of metric vs swept variable (small p = metric moves with the parameter):")
    table(quality[["experiment", "pipeline", "fail_chi2", "fail_entropy", "fail_npcr_ch", "fail_uaci",
                   "chi2_p_ks", "spearman_p_npcr_ch_vs_x", "spearman_p_uaci_vs_x",
                   "spearman_p_chi2_vs_x", "spearman_p_avalanche_vs_x"]])
    table(quality[["experiment", "pipeline", "npcr_any_mean", "npcr_ch_mean", "uaci_mean",
                   "entropy_mean", "avalanche_mean", "avalanche_z_mean", "avalanche_z_sd"]])
    prop = quality[quality["pipeline"] == "proposed"]
    say(f"ACM identity (proposed pixel map, period P(N={AE_N}) divides n): " + ", ".join(
        f"{r.experiment} {int(r.acm_identity_rows)} of {r.n}" for r in prop.itertuples()))
    say("Seed chains (a-d one chain from s, e each row from its key's s): "
        + ("; ".join(issues) if issues else "no issues"))
    say()
    return fits, data, issues


# --- figures ---------------------------------------------------------------------------------

def style(ax):
    ax.set_facecolor(SURFACE)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(GRID)
    ax.tick_params(colors=INK_MUTED, labelsize=8)
    ax.grid(True, color=GRID, linewidth=0.6)
    ax.set_axisbelow(True)
    ax.xaxis.label.set_color(INK)
    ax.yaxis.label.set_color(INK)
    ax.title.set_color(INK)


def figure(nrows, ncols, size):
    fig, axes = plt.subplots(nrows, ncols, figsize=size, facecolor=SURFACE, squeeze=False)
    for ax in axes.flat:
        style(ax)
    return fig, axes


def save(fig, name):
    fig.tight_layout()
    fig.savefig(OUT / name, dpi=200, facecolor=SURFACE)
    plt.close(fig)


def plot_timing(f, fits):
    fig, axes = figure(1, 2, (10, 4))
    mb = f["size_bytes"] / 1e6
    xs = np.linspace(0, mb.max(), 50)
    for ax, op, title in zip(axes[0], ["enc", "dec"], ["Encrypt", "Decrypt"]):
        for pipe in PIPELINES:
            fit = fits[(fits["pipeline"] == pipe) & (fits["op"] == op)].iloc[0]
            ax.scatter(mb, f[f"{op}_{pipe}_s"], s=9, color=COLOR[pipe], alpha=0.55, linewidths=0,
                       label=f"{pipe} ({fit.slope:.2f} s/MB)")
            ax.plot(xs, fit.intercept + fit.slope * xs, color=COLOR[pipe], linewidth=2)
        ax.set_title(f"{title} time vs plainfile size (corpus f)", fontsize=10, loc="left")
        ax.set_xlabel("Plainfile size (MB)")
        ax.set_ylabel("Seconds")
        ax.legend(frameon=False, fontsize=8, labelcolor=INK)
    save(fig, "f_timing.png")


def plot_entropy(f, keep):
    """Entropy shortfall 8 - H vs size: both scale as 1/size, so log-log makes them comparable."""
    sub = f[keep].sort_values("size_bytes")
    kb = sub["size_bytes"] / 1e3
    samples = 3 * sub["width"] * sub["height"]
    expected = CHI2_DF / (2 * samples * math.log(2))
    fig, axes = figure(2, 2, (10, 7.5))
    for col, pipe in enumerate(PIPELINES):
        short = 8 - sub[f"entropy_{pipe}"]
        fails = int((sub[f"entropy_{pipe}"] < sub["entropy_crit"]).sum())
        # top: raw shortfall, which shrinks as 1/size
        ax = axes[0, col]
        ax.scatter(kb, short, s=9, color=COLOR[pipe], alpha=0.6, linewidths=0, label=f"{pipe} files")
        ax.plot(kb, 8 - sub["entropy_crit"], color=INK, linewidth=1.5,
                label="critical value, alpha 0.05 (fail above)")
        ax.plot(kb, expected, color=INK_MUTED, linewidth=1.5, linestyle="--",
                label="expected under uniformity")
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_title(f"{pipe}: {fails} of {len(sub)} files fail", fontsize=10, loc="left")
        ax.legend(frameon=False, fontsize=8, labelcolor=INK, loc="lower left")
        # bottom: shortfall / expected, so the critical value is one flat line for every size
        ax = axes[1, col]
        ax.scatter(kb, short / expected, s=9, color=COLOR[pipe], alpha=0.6, linewidths=0)
        ax.axhline(CHI2_CRIT / CHI2_DF, color=INK, linewidth=1.5)
        ax.axhline(1, color=INK_MUTED, linewidth=1.5, linestyle="--")
        ax.set_xscale("log")
        ax.set_xlabel("Plainfile size (KB)")
    axes[0, 0].set_ylabel("Entropy shortfall 8 - H (bits)")
    axes[1, 0].set_ylabel("Shortfall / expected shortfall")
    axes[0, 1].sharey(axes[0, 0])
    axes[1, 1].sharey(axes[1, 0])
    fig.suptitle("Cipherimage entropy shortfall vs plainfile size (corpus f)",
                 x=0.01, ha="left", fontsize=10, color=INK)
    save(fig, "f_entropy.png")


def plot_pvalues(f, keep, zs):
    fig, axes = figure(2, len(TYPE_ORDER), (14, 4.2))
    bins = np.linspace(0, 1, 11)
    for row, pipe in enumerate(PIPELINES):
        for col, t in enumerate(TYPE_ORDER):
            ax = axes[row, col]
            p = zs[pipe].loc[keep & (f["type"] == t), "chi2_p"]
            ax.hist(p, bins=bins, color=COLOR[pipe], edgecolor=SURFACE, linewidth=2)
            ax.axhline(len(p) / 10, color=INK_MUTED, linewidth=1, linestyle="--")
            ax.set_xlim(0, 1)
            if row == 0:
                ax.set_title(t, fontsize=9, loc="left")
            if col == 0:
                ax.set_ylabel(f"{pipe}\nfiles", fontsize=8)
            if row == 1:
                ax.set_xlabel("chi2 p-value", fontsize=8)
    fig.suptitle("Cipherimage chi2 p-values per type (dashed: uniform expectation)",
                 x=0.01, ha="left", fontsize=10, color=INK)
    save(fig, "f_chi2_pvalues.png")


def plot_plain_vs_cipher(f, keep, plain_keep):
    """Per-file chi2 and entropy by type: plainfile next to both cipherimages (H1, H4)."""
    passes = chi2_passes(f, keep, plain_keep)
    series = [("plain", "chi2_plain", "entropy_plain", plain_keep, "plainfile")] + [
        (pipe, f"chi2_{pipe}", f"entropy_{pipe}", keep, f"{pipe} cipherimage") for pipe in PIPELINES]
    fig, axes = figure(2, 1, (11, 8))
    rng = np.random.default_rng(0)
    for ax, metric in zip(axes[:, 0], ["chi2", "entropy"]):
        for i, t in enumerate(TYPE_ORDER):
            for j, (key, chi2_col, entropy_col, mask, label) in enumerate(series):
                y = f.loc[mask & (f["type"] == t), chi2_col if metric == "chi2" else entropy_col]
                x = i + (j - 1) * 0.27 + rng.uniform(-0.09, 0.09, len(y))
                if metric == "chi2":
                    k, n = passes[key]
                    label = f"{label} ({k} of {n} pass)"
                ax.scatter(x, y, s=7, color=COLOR[key], alpha=0.6, linewidths=0,
                           label=label if i == 0 else None)
        ax.set_xticks(range(len(TYPE_ORDER)))
        ax.set_xticklabels(TYPE_ORDER, fontsize=8)
    ax = axes[0, 0]
    ax.set_yscale("log")
    ax.set_ylim(top=f.loc[plain_keep, "chi2_plain"].max() * 300)  # headroom for the legend
    ax.axhline(CHI2_CRIT, color=INK, linewidth=1.5, label=f"critical value {CHI2_CRIT}, alpha 0.05 (fail above)")
    ax.axhline(CHI2_DF, color=INK_MUTED, linewidth=1.5, linestyle="--", label="expected under uniformity (255)")
    ax.set_ylabel("chi2 (df 255, log scale)")
    ax.set_title("Byte-histogram chi2 per file", fontsize=10, loc="left")
    ax.legend(frameon=False, fontsize=8, labelcolor=INK, loc="upper right", ncol=2, markerscale=2)
    ax = axes[1, 0]
    ax.axhline(8, color=INK_MUTED, linewidth=1.5, linestyle="--", label="ideal (8 bits)")
    ax.set_ylabel("Entropy (bits per byte)")
    ax.set_title("Byte-histogram entropy per file (pass/fail by size: f_entropy.png)", fontsize=10, loc="left")
    ax.legend(frameon=False, fontsize=8, labelcolor=INK, loc="lower right", markerscale=2)
    fig.suptitle("Plainfile vs cipherimage uniformity by file type (corpus f)",
                 x=0.01, ha="left", fontsize=10, color=INK)
    save(fig, "f_plain_vs_cipher.png")


def plot_z(f, keep, zs):
    stats_ = [("chi2_p", "chi2 p-value"), ("npcr_ch_z", "NPCR_ch z"), ("uaci_z", "UACI z"),
              ("avalanche_z", "Avalanche z")]
    fig, axes = figure(1, 4, (15, 3.8))
    rng = np.random.default_rng(0)
    for ax, (col, label) in zip(axes[0], stats_):
        if col == "chi2_p":
            ax.axhspan(0, ALPHA, color=GRID, linewidth=0)
        else:
            ax.axhspan(-1.96, 1.96, color=GRID, linewidth=0)
        for i, t in enumerate(TYPE_ORDER):
            for j, pipe in enumerate(PIPELINES):
                y = zs[pipe].loc[keep & (f["type"] == t), col]
                x = i + (j - 0.5) * 0.36 + rng.uniform(-0.12, 0.12, len(y))
                ax.scatter(x, y, s=6, color=COLOR[pipe], alpha=0.6, linewidths=0,
                           label=pipe if i == 0 else None)
        ax.set_xticks(range(len(TYPE_ORDER)))
        ax.set_xticklabels(TYPE_ORDER, fontsize=8)
        ax.set_title(label, fontsize=10, loc="left")
    fig.suptitle("Per-file size-free statistics by type (band: chi2 p < 0.05 / |z| < 1.96)",
                 x=0.01, ha="left", fontsize=10, color=INK)
    fig.legend(*axes[0, 0].get_legend_handles_labels(), loc="upper right", ncol=2, frameon=False,
               fontsize=8, labelcolor=INK, markerscale=2)
    save(fig, "f_statistics_by_type.png")


def plot_ae(data):
    fig, axes = figure(1, 5, (18, 3.6))
    for ax, (letter, (xcol, _)) in zip(axes[0], AE.items()):
        d = data[letter]
        for pipe in PIPELINES:
            ax.plot(d[xcol], d[f"enc_{pipe}_s"], color=COLOR[pipe], linewidth=2, marker="o", markersize=3,
                    label=f"{pipe} encrypt")
            ax.plot(d[xcol], d[f"dec_{pipe}_s"], color=COLOR[pipe], linewidth=2, linestyle="--",
                    label=f"{pipe} decrypt")
        ax.set_title(f"{letter}: time vs {xcol}", fontsize=10, loc="left")
        ax.set_xlabel(xcol)
        ax.set_ylim(bottom=0)
    axes[0, 0].set_ylabel("Seconds")
    axes[0, 0].legend(frameon=False, fontsize=7, labelcolor=INK)
    save(fig, "ae_timing.png")


# --- main ------------------------------------------------------------------------------------

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    f = pd.read_csv(RESULTS / "f.csv", dtype=SEED_DTYPES)
    say("FTIE experiment analysis (experiment/analyze.py)")
    say(f"avalanche null pair variance: {AVALANCHE_VAR_PAIR:.10g} bits^2 per 16-bit pair\n")

    issues = check_integrity(f)
    keep, plain_keep = exclusions(f)
    zs = {pipe: z_scores(f, pipe, f["width"]) for pipe in PIPELINES}
    fails = {pipe: per_file_tests(f, pipe) for pipe in PIPELINES}

    per_file = f[["type", "subset", "file_name", "sha256", "size_bytes", "width"]].copy()
    per_file["included"] = keep
    params = read_params(RESULTS / "f.csv.params")
    per_file["acm_period"], per_file["acm_identity"] = acm_identity(
        pd.Series(int(params["acm_a"]), index=f.index), pd.Series(int(params["acm_b"]), index=f.index),
        int(params["acm_n"]), f["width"])
    identity = per_file[per_file["acm_identity"]]
    say(f"ACM identity (proposed pixel map, period P(N) divides n = {params['acm_n']}): "
        f"{len(identity)} of {len(f)} files" + (": " + ", ".join(
            f"{r.file_name} (N={r.width}, P={r.acm_period})" for r in identity.itertuples())
            if len(identity) else "") + "\n")
    for pipe in PIPELINES:
        for col in zs[pipe]:
            per_file[f"{col}_{pipe}"] = zs[pipe][col]
        for test in TESTS:
            per_file[f"fail_{test}_{pipe}"] = fails[pipe][test]
    write_csv(per_file, "f_per_file.csv")

    decisions(f, keep, fails)
    uniformity(f, keep, zs)
    describe(f, keep, plain_keep, zs)
    f_fits = timing_fits(f)
    ae_fits, ae_data, ae_issues = analyse_ae()
    issues += ae_issues
    write_csv(pd.concat([f_fits, ae_fits], ignore_index=True), "timing_fits.csv")

    plot_timing(f, f_fits)
    plot_entropy(f, keep)
    plot_pvalues(f, keep, zs)
    plot_plain_vs_cipher(f, keep, plain_keep)
    plot_z(f, keep, zs)
    plot_ae(ae_data)

    (OUT / "report.txt").write_text("\n".join(report_lines) + "\n")
    print(f"\nwrote {OUT.relative_to(ROOT)}/")
    return 1 if issues else 0


if __name__ == "__main__":
    sys.exit(main())
