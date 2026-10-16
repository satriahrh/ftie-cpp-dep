#!/usr/bin/env bash
#
# One-time setup for a fresh Ubuntu 24.04 EC2 instance: installs the toolchain
# the experiments need (g++, libpng-dev, libpng++-dev, libgmp-dev, git) plus
# the tools experiment/RUNBOOK.md uses (tmux, unzip, curl), verifies each
# dependency actually landed, then prints next steps.
#
# Run this once after SSH'ing into the instance:
#   ./setup_vps.sh

set -uo pipefail

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
  SUDO="sudo"
fi

echo "installing packages: build-essential libpng-dev libpng++-dev libgmp-dev git tmux unzip curl"
$SUDO apt-get update
$SUDO apt-get install -y build-essential libpng-dev libpng++-dev libgmp-dev git tmux unzip curl

FAILED=0

check() {
  local desc="$1"
  shift
  if "$@" >/dev/null 2>&1; then
    echo "OK   $desc"
  else
    echo "FAIL $desc"
    FAILED=1
  fi
}

echo
echo "verifying toolchain:"
check "g++ on PATH"            command -v g++
check "make on PATH"           command -v make
check "git on PATH"            command -v git
check "libpng-config on PATH"  command -v libpng-config
check "gmpxx.h present"        test -f /usr/include/gmpxx.h
check "png++/png.hpp present"  test -f /usr/include/png++/png.hpp
check "tmux on PATH"           command -v tmux
check "unzip on PATH"          command -v unzip
check "curl on PATH"           command -v curl

echo
if [ "$FAILED" -ne 0 ]; then
  echo "one or more dependencies are missing (see FAIL lines above) - fix before proceeding."
  exit 1
fi

echo "all dependencies OK. Next: follow experiment/RUNBOOK.md from step 1.2 (start tmux)."
