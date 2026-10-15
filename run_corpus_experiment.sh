#!/usr/bin/env bash
#
# Builds and runs the corpus-driven experiment/f.cpp harness, separately
# from run_experiments.sh (which only ever touches a-e). Appends O5's run
# record (git commit, compiler version, run date) to the same
# "<output-csv>.params" sidecar f.cpp uses to freeze its BBS/ACM
# parameters -- those parameter lines are written once by the binary and
# never touched here; this script only ever appends/refreshes its own
# three lines.
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

mkdir -p bin experiment/dum "$(dirname "$OUT")"

command -v g++ >/dev/null 2>&1 || { echo "g++ not found on PATH" >&2; exit 1; }
command -v libpng-config >/dev/null 2>&1 || { echo "libpng-config not found on PATH" >&2; exit 1; }

echo "building experiment/f.cpp (make experiment-build-f)"
if ! make experiment-build-f; then
  echo "BUILD FAILED - aborting" >&2
  exit 1
fi

echo "running corpus experiment (manifest=$MANIFEST, output=$OUT)"
make experiment-run-f MANIFEST="$MANIFEST" OUT="$OUT"
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
