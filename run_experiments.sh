#!/usr/bin/env bash
#
# Builds and runs the experiment/{a..e}.cpp parameter-sweep harnesses sequentially,
# saving each experiment's output to its own file under experiment/results/.
# Experiment f (corpus files) has its own runner: run_corpus_experiment.sh.
#
# Usage:
#   ./run_experiments.sh                # run all five, in order, skipping ones already done
#   ./run_experiments.sh d e            # run only d and e
#   ./run_experiments.sh --force        # re-run all five, keeping experiment/bbs_params.csv
#   ./run_experiments.sh --force e      # re-run only e
#   ./run_experiments.sh --new-params   # draw new BBS keys, then re-run (implies --force)
#
# BBS keys (experiment/bbs_params.csv) are generated only when missing, so a
# resumed or forced run keeps the keys every finished experiment used; commit
# that file with the results. Each experiment writes <letter>.csv.partial and
# renames it to <letter>.csv only when it exits 0, so an interrupted
# experiment is never mistaken for a finished one -- just run the script again.
#
# Meant for a long-running unattended session -- run it under tmux/screen or
# `nohup ./run_experiments.sh > /dev/null 2>&1 &` so it survives an SSH disconnect.
# Monitor progress with: tail -f experiment/results/run.log
#                         tail -f experiment/results/<letter>.csv.partial
# RESULTS_DIR overrides experiment/results (e.g. for a test run).

set -uo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

RESULTS_DIR="${RESULTS_DIR:-experiment/results}"
BBS_PARAMS="experiment/bbs_params.csv"

FORCE=0
NEW_PARAMS=0
LETTERS=()
for arg in "$@"; do
  case "$arg" in
    --force) FORCE=1 ;;
    --new-params) NEW_PARAMS=1; FORCE=1 ;;
    a|b|c|d|e) LETTERS+=("$arg") ;;
    f) echo "experiment f runs via ./run_corpus_experiment.sh" >&2; exit 1 ;;
    *) echo "unknown argument: $arg (expected --force, --new-params and/or letters a-e)" >&2; exit 1 ;;
  esac
done
if [ ${#LETTERS[@]} -eq 0 ]; then
  LETTERS=(a b c d e)
fi

mkdir -p bin experiment/dum "$RESULTS_DIR"

log() {
  local msg
  msg="[$(date '+%Y-%m-%d %H:%M:%S')] $*"
  echo "$msg"
  echo "$msg" >> "$RESULTS_DIR/run.log"
}

command -v g++ >/dev/null 2>&1 || { echo "g++ not found on PATH" >&2; exit 1; }
command -v libpng-config >/dev/null 2>&1 || { echo "libpng-config not found on PATH" >&2; exit 1; }

log "=== run_experiments.sh starting (letters: ${LETTERS[*]}, force=$FORCE, new_params=$NEW_PARAMS) ==="
experiment/record_env.sh "run_experiments.sh ${LETTERS[*]}" "$RESULTS_DIR/env.txt"

if [ ! -s "$BBS_PARAMS" ] || [ "$NEW_PARAMS" -eq 1 ]; then
  log "generating BBS params (make experiment-gen-params)"
  if make experiment-gen-params > "$RESULTS_DIR/gen-params.log" 2>&1; then
    log "BBS params generated OK"
  else
    log "FAILED to generate BBS params (see $RESULTS_DIR/gen-params.log) - aborting"
    exit 1
  fi
else
  log "reusing existing $BBS_PARAMS"
fi

log "building all experiment binaries (make experiment-build-all)"
if make experiment-build-all > "$RESULTS_DIR/build.log" 2>&1; then
  log "build OK"
else
  log "BUILD FAILED (see $RESULTS_DIR/build.log) - aborting"
  exit 1
fi

SUMMARY=()

for X in "${LETTERS[@]}"; do
  out="$RESULTS_DIR/$X.csv"
  partial="$out.partial"
  err="$RESULTS_DIR/$X.stderr.log"

  if [ -s "$out" ] && [ "$FORCE" -ne 1 ]; then
    log "skipping experiment $X (output already exists: $out, use --force to redo)"
    SUMMARY+=("  $X: skipped  output=$out")
    continue
  fi

  log "starting experiment $X"
  start=$(date +%s)

  # Drop any earlier result (e.g. the pilot's, tracked in git) first, so an
  # interrupted run leaves no stale <letter>.csv to be skipped next time.
  rm -f "$out"
  # The binary prints its own header and flushes every row.
  "./bin/exp-$X.o" > "$partial" 2> "$err"
  code=$?

  dur=$(( $(date +%s) - start ))

  if [ $code -eq 0 ]; then
    mv "$partial" "$out"
    log "finished experiment $X in ${dur}s (exit 0)"
    SUMMARY+=("  $X: ok  duration=${dur}s  output=$out")
  else
    log "experiment $X FAILED after ${dur}s (exit $code, see $err; partial rows in $partial)"
    SUMMARY+=("  $X: failed(exit $code)  duration=${dur}s  partial=$partial")
  fi
done

log "=== summary ==="
for line in "${SUMMARY[@]}"; do
  log "$line"
done
log "=== run_experiments.sh done ==="
