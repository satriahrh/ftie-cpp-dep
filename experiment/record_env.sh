#!/usr/bin/env bash
#
# Appends one dated block describing the machine and code state to the
# experiment environment record (default experiment/results/env.txt), so
# every invocation of run_experiments.sh / run_corpus_experiment.sh -- a
# first run or a resume -- is recorded. Called by both scripts.
#
# Usage: experiment/record_env.sh <label> [env-file]

set -uo pipefail

LABEL="${1:-manual}"
ENV_FILE="${2:-experiment/results/env.txt}"
mkdir -p "$(dirname "$ENV_FILE")"

ec2_instance_type() {
  # IMDSv2; 1 s timeouts so this is a no-op off EC2.
  local token
  token="$(curl -s -m 1 -X PUT http://169.254.169.254/latest/api/token \
    -H "X-aws-ec2-metadata-token-ttl-seconds: 60" 2>/dev/null)" || return 0
  [ -n "$token" ] || return 0
  curl -s -m 1 -H "X-aws-ec2-metadata-token: $token" \
    http://169.254.169.254/latest/meta-data/instance-type 2>/dev/null
}

{
  echo "=== $(date -u '+%Y-%m-%dT%H:%M:%SZ') $LABEL"
  echo "git_commit=$(git rev-parse HEAD 2>/dev/null || echo unknown)"
  # Code state only: results under experiment/results are expected to change.
  if [ -n "$(git status --porcelain --untracked-files=no -- . ':!experiment/results' 2>/dev/null)" ]; then
    echo "git_dirty=yes (uncommitted changes to tracked files)"
  else
    echo "git_dirty=no"
  fi
  echo "compiler=$(g++ --version 2>/dev/null | head -n1)"
  echo "instance_type=$(ec2_instance_type)"
  echo "uname=$(uname -a)"
  if command -v lscpu >/dev/null 2>&1; then
    lscpu
  else
    sysctl -n machdep.cpu.brand_string hw.ncpu hw.memsize 2>/dev/null
  fi
  echo
} >> "$ENV_FILE"
