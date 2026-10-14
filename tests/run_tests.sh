#!/usr/bin/env bash
# For every tests/*.c: emit unoptimized IR with Clang, run mem2reg (+ fold-dce),
# then check (a) the IR got smaller and (b) the program output is unchanged.
set -euo pipefail

LLVM_BIN="${LLVM_BIN:-/usr/lib/llvm-18/bin}"
HERE="$(cd "$(dirname "$0")" && pwd)"
PLUGIN="${PLUGIN:-$HERE/../build/FoldDCE.so}"
OUT="$HERE/../build/test-out"
mkdir -p "$OUT"

count_insts() {  # number of instructions in a textual .ll file
  grep -E '^\s+(%[A-Za-z0-9_.]+ = |store |br |ret |call |switch |unreachable)' "$1" | wc -l
}

pass=0; fail=0
printf "%-14s %8s %8s %8s  %s\n" TEST BEFORE AFTER REMOVED RESULT
for src in "$HERE"/*.c; do
  name="$(basename "$src" .c)"
  "$LLVM_BIN/clang" -O0 -Xclang -disable-O0-optnone -S -emit-llvm "$src" -o "$OUT/$name.O0.ll"
  "$LLVM_BIN/opt" -passes='mem2reg' -S "$OUT/$name.O0.ll" -o "$OUT/$name.before.ll"
  "$LLVM_BIN/opt" -load-pass-plugin="$PLUGIN" -passes='mem2reg,fold-dce' \
        -fold-dce-verbose -S "$OUT/$name.O0.ll" -o "$OUT/$name.after.ll" 2>"$OUT/$name.log"
  "$LLVM_BIN/opt" -passes=verify -disable-output "$OUT/$name.after.ll"   # IR must stay valid

  b=$(count_insts "$OUT/$name.before.ll"); a=$(count_insts "$OUT/$name.after.ll")
  exp="$("$LLVM_BIN/lli" "$OUT/$name.before.ll")"
  got="$("$LLVM_BIN/lli" "$OUT/$name.after.ll")"

  if [[ "$exp" == "$got" && "$a" -lt "$b" ]]; then res=PASS; pass=$((pass+1)); else res=FAIL; fail=$((fail+1)); fi
  printf "%-14s %8d %8d %8d  %s\n" "$name" "$b" "$a" "$((b-a))" "$res"
done
echo "passed=$pass failed=$fail"
[[ $fail -eq 0 ]]
