#!/usr/bin/env bash
#
# Builds and runs the corpus-driven experiment/f.cpp harness, separately
# from run_experiments.sh (which only ever touches a-e). Appends O5's run
# record (git commit, compiler version, run date) to the same
# "<output-csv>.params" sidecar f.cpp uses to freeze its BBS/ACM
# parameters -- those parameter lines are written once by the binary and
# never touched here; this script only ever appends/refreshes its own
# three lines. Every invocation also appends a block to env.txt next to the
# output CSV (experiment/record_env.sh), and the binary's stderr (done /
# skip / FAILED lines) to <output-csv minus .csv>.stderr.log.
#
# Usage:
#   ./run_corpus_experiment.sh                                   # defaults: experiment/corpus_manifest.csv -> experiment/results/f.csv
#   ./run_corpus_experiment.sh <manifest-csv> <output-csv>        # explicit paths (e.g. a validation run under experiment/dum/)
#
# Safe to re-run: f.cpp resumes from <output-csv>, skipping files already
# present (matched by sha256), using the same frozen params every time.

set -uo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

MANIFEST="${1:-experiment/corpus_manifest.csv}"
OUT="${2:-experiment/results/f.csv}"
PARAMS="$OUT.params"
LOG="${OUT%.csv}.stderr.log"
BBS_PARAMS="experiment/bbs_params.csv"

mkdir -p bin experiment/dum "$(dirname "$OUT")"

command -v g++ >/dev/null 2>&1 || { echo "g++ not found on PATH" >&2; exit 1; }
command -v libpng-config >/dev/null 2>&1 || { echo "libpng-config not found on PATH" >&2; exit 1; }

experiment/record_env.sh "run_corpus_experiment.sh $MANIFEST -> $OUT" "$(dirname "$OUT")/env.txt"

# Same rule as run_experiments.sh: generate BBS keys only when missing, so
# a-e and f share one bbs_params.csv whichever runs first.
if [ ! -s "$BBS_PARAMS" ]; then
  echo "generating BBS params (make experiment-gen-params)"
  if ! make experiment-gen-params; then
    echo "FAILED to generate BBS params - aborting" >&2
    exit 1
  fi
fi

echo "building experiment/f.cpp (make experiment-build-f)"
if ! make experiment-build-f; then
  echo "BUILD FAILED - aborting" >&2
  exit 1
fi

echo "running corpus experiment (manifest=$MANIFEST, output=$OUT, stderr also in $LOG)"
echo "=== $(date -u '+%Y-%m-%dT%H:%M:%SZ') run_corpus_experiment.sh $MANIFEST -> $OUT" >> "$LOG"
make experiment-run-f MANIFEST="$MANIFEST" OUT="$OUT" 2> >(tee -a "$LOG" >&2)
code=$?

# O5 run record: refresh these three lines every invocation (the BBS/ACM
# lines above them are frozen by f.cpp on first run and must not be
# touched here).
if [ -f "$PARAMS" ]; then
  grep -v -e '^git_commit=' -e '^compiler=' -e '^run_date=' "$PARAMS" > "$PARAMS.tmp" || true
  mv "$PARAMS.tmp" "$PARAMS"
  {
    echo "git_commit=$(git rev-parse HEAD 2>/dev/null || echo unknown)"
    echo "compiler=$(g++ --version | head -n1)"
    echo "run_date=$(date '+%Y-%m-%d')"
  } >> "$PARAMS"
fi

exit $code
