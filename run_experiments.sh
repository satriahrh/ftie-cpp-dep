#!/usr/bin/env bash
#
# Builds and runs the experiment/{a..f}.cpp comparison harnesses sequentially,
# saving each experiment's output to its own file under experiment/results/.
#
# Usage:
#   ./run_experiments.sh              # run all six, in order, skipping ones already done
#   ./run_experiments.sh e f          # run only e and f
#   ./run_experiments.sh --force      # re-run everything (params, build, all experiments)
#   ./run_experiments.sh --force e    # re-run only e
#
# Meant for a long-running unattended VPS session — run it under tmux/screen or
# `nohup ./run_experiments.sh > /dev/null 2>&1 &` so it survives an SSH disconnect.
# Monitor progress with: tail -f experiment/results/run.log
#                         tail -f experiment/results/<letter>.csv

set -uo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

RESULTS_DIR="experiment/results"

FORCE=0
LETTERS=()
for arg in "$@"; do
  case "$arg" in
    --force) FORCE=1 ;;
    a|b|c|d|e|f) LETTERS+=("$arg") ;;
    *) echo "unknown argument: $arg (expected --force and/or letters a-f)" >&2; exit 1 ;;
  esac
done
if [ ${#LETTERS[@]} -eq 0 ]; then
  LETTERS=(a b c d e f)
fi

# per-experiment CSV header; only the first column ("x") differs in meaning per file
header_for() {
  local x_col
  case "$1" in
    a|b) x_col="n" ;;
    c|d) x_col="a_param" ;;
    e)   x_col="bits" ;;
    f)   x_col="size_mb" ;;
  esac
  echo "$x_col,npcr_dep,entropy_dep,avalanche_dep,enc_dep_s,dec_dep_s,npcr_cur,entropy_cur,avalanche_cur,enc_cur_s,dec_cur_s"
}

mkdir -p bin experiment/dum "$RESULTS_DIR"

log() {
  local msg
  msg="[$(date '+%Y-%m-%d %H:%M:%S')] $*"
  echo "$msg"
  echo "$msg" >> "$RESULTS_DIR/run.log"
}

command -v g++ >/dev/null 2>&1 || { echo "g++ not found on PATH" >&2; exit 1; }
command -v libpng-config >/dev/null 2>&1 || { echo "libpng-config not found on PATH" >&2; exit 1; }

log "=== run_experiments.sh starting (letters: ${LETTERS[*]}, force=$FORCE) ==="

if [ ! -s experiment/bbs_params.csv ] || [ "$FORCE" -eq 1 ]; then
  log "generating BBS params (make experiment-gen-params)"
  if make experiment-gen-params > "$RESULTS_DIR/gen-params.log" 2>&1; then
    log "BBS params generated OK"
  else
    log "FAILED to generate BBS params (see $RESULTS_DIR/gen-params.log) - aborting"
    exit 1
  fi
else
  log "reusing existing experiment/bbs_params.csv"
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
  err="$RESULTS_DIR/$X.stderr.log"

  if [ -s "$out" ] && [ "$FORCE" -ne 1 ]; then
    log "skipping experiment $X (output already exists: $out, use --force to redo)"
    SUMMARY+=("  $X: skipped  output=$out")
    continue
  fi

  log "starting experiment $X"
  start=$(date +%s)

  header_for "$X" > "$out"
  stdbuf -oL "./bin/exp-$X.o" >> "$out" 2> "$err"
  code=$?

  dur=$(( $(date +%s) - start ))

  if [ $code -eq 0 ]; then
    log "finished experiment $X in ${dur}s (exit 0)"
    SUMMARY+=("  $X: ok  duration=${dur}s  output=$out")
  else
    log "experiment $X FAILED after ${dur}s (exit $code, see $err)"
    SUMMARY+=("  $X: failed(exit $code)  duration=${dur}s  output=$out")
  fi
done

log "=== summary ==="
for line in "${SUMMARY[@]}"; do
  log "$line"
done
log "=== run_experiments.sh done ==="
