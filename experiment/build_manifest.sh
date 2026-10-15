#!/usr/bin/env bash
#
# Builds the corpus manifest experiment/f.cpp reads, from extracted
# NapierOne Tiny archives laid out one directory per subset:
#
#   <corpus-root>/DOCX/...          <corpus-root>/MP3/...
#   <corpus-root>/XLSX/...          <corpus-root>/MP4-FROM-WEB/...
#   <corpus-root>/PPTX/...          <corpus-root>/JPG-FROM-WEB/...
#   <corpus-root>/PDF/...
#
# Keeps only the numbered corpus files (names starting "NNNN-", which drops
# the per-subset documentation PDF such as DOCX.pdf) up to CAP bytes
# (default 10,000,000; set CAP=10485760 for 10 MiB), and writes one row per
# file sorted by subset then name, with its SHA-256 (requirements D2-D5, D7).
# Excluded files and per-type counts go to stderr.
#
# Usage:
#   experiment/build_manifest.sh <corpus-root> <download-date YYYY-MM-DD> [output-csv]
#   default output: experiment/corpus_manifest.csv
#
# Use an absolute <corpus-root>: f.cpp opens each path as written.

set -euo pipefail

if [ $# -lt 2 ]; then
  echo "usage: $0 <corpus-root> <download-date YYYY-MM-DD> [output-csv]" >&2
  exit 1
fi

ROOT="${1%/}"
DOWNLOAD_DATE="$2"
OUT="${3:-experiment/corpus_manifest.csv}"
CAP="${CAP:-10000000}"
TIER="tiny"

if command -v sha256sum >/dev/null 2>&1; then
  sha256() { sha256sum "$1" | cut -d' ' -f1; }
else
  sha256() { shasum -a 256 "$1" | cut -d' ' -f1; }
fi

# subset directory -> original FTIE file type
SUBSETS=(DOCX XLSX PPTX PDF MP3 MP4-FROM-WEB JPG-FROM-WEB)
type_for() {
  case "$1" in
    MP4-FROM-WEB) echo MP4 ;;
    JPG-FROM-WEB) echo JPG ;;
    *) echo "$1" ;;
  esac
}

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT
echo "type,subset,file_name,path,size_bytes,sha256,archive_tier,download_date" > "$tmp"

missing=0
for subset in "${SUBSETS[@]}"; do
  dir="$ROOT/$subset"
  if [ ! -d "$dir" ]; then
    echo "MISSING subset directory: $dir" >&2
    missing=1
    continue
  fi

  kept=0
  while IFS= read -r path; do
    name="$(basename "$path")"
    if ! [[ "$name" =~ ^[0-9]{4}- ]]; then
      echo "exclude (not a numbered corpus file): $subset/$name" >&2
      continue
    fi
    if [[ "$path" == *,* ]]; then
      echo "ERROR: comma in path (manifest is unquoted CSV): $path" >&2
      exit 1
    fi
    size="$(wc -c < "$path" | tr -d ' ')"
    if [ "$size" -gt "$CAP" ]; then
      echo "exclude (over cap $CAP B, $size B): $subset/$name" >&2
      continue
    fi
    echo "$(type_for "$subset"),$subset,$name,$path,$size,$(sha256 "$path"),$TIER,$DOWNLOAD_DATE" >> "$tmp"
    kept=$((kept + 1))
  done < <(find "$dir" -type f | LC_ALL=C sort)

  echo "$subset: $kept files" >&2
done

if [ "$missing" -ne 0 ]; then
  echo "one or more subset directories are missing - manifest not written" >&2
  exit 1
fi

mkdir -p "$(dirname "$OUT")"
mv "$tmp" "$OUT"
trap - EXIT
echo "wrote $OUT ($(($(wc -l < "$OUT") - 1)) files)" >&2
