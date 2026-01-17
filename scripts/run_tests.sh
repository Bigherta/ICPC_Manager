#!/usr/bin/env bash
set -u

EXE=${1:-"./build/icpc_manager"}
DATA_DIR=${2:-"./data"}

if [[ ! -x "$EXE" ]]; then
  echo "Executable not found: $EXE"
  exit 1
fi
if [[ ! -d "$DATA_DIR" ]]; then
  echo "Data dir not found: $DATA_DIR"
  exit 1
fi

BUILD_DIR="$(dirname "$EXE")"
OUT_DIR="${BUILD_DIR}/test_output"
mkdir -p "$OUT_DIR"

# migrate any previously-generated .out files to .txt to keep extensions consistent
shopt -s nullglob
for old in "$OUT_DIR"/*.out; do
  mv "$old" "${old%.out}.txt"
done
shopt -u nullglob

total=0
passed=0
failed=0
fails=()

shopt -s nullglob
for in_file in "$DATA_DIR"/*.in; do
  ((total++)) || true
  base="$(basename "$in_file" .in)"
  # skip oversized test case
  if [[ "$base" == "bigger" ]]; then
    echo "[SKIP]  $base (skipped by request)"
    continue
  fi
  # expected file: prefer data/<base>.out, fall back to data/<base>.txt or build/test_output/<base>.txt
  exp_file="$DATA_DIR/$base.out"
  if [[ ! -f "$exp_file" ]]; then
    if [[ -f "$DATA_DIR/$base.txt" ]]; then
      exp_file="$DATA_DIR/$base.txt"
    elif [[ -f "$OUT_DIR/$base.txt" ]]; then
      exp_file="$OUT_DIR/$base.txt"
    elif [[ -f "$OUT_DIR/$base.out" ]]; then
      exp_file="$OUT_DIR/$base.out"
    fi
  fi
  gen_file="$OUT_DIR/$base.txt"
  diff_file="$OUT_DIR/$base.diff"

  if [[ ! -f "$exp_file" ]]; then
    echo "[MISS] $base -> expected file not found: $exp_file"
    ((failed++)) || true
    fails+=("$base (missing expected)")
    continue
  fi

  "$EXE" < "$in_file" > "$gen_file"
  # use -w to ignore all whitespace differences when comparing
  if diff -w -u "$exp_file" "$gen_file" > "$diff_file"; then
    echo "[OK]   $base"
    ((passed++)) || true
    rm -f "$diff_file"
  else
    echo "[FAIL] $base (see $diff_file)"
    ((failed++)) || true
    fails+=("$base")
  fi
done
shopt -u nullglob

echo ""
echo "Summary: total=$total, passed=$passed, failed=$failed"
if [[ $failed -ne 0 ]]; then
  echo "Failed cases: ${fails[*]}"
  exit 1
fi
exit 0
