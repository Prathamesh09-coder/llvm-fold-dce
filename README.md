# LLVM Fold-DCE: Constant Folding & Dead-Code Elimination Pass (C++)

An out-of-tree **LLVM function pass** (new pass manager, loaded as a plugin) written in C++.
For every function it repeats, until a fixed point is reached:

1. **Constant folding** - evaluates instructions whose operands are all constants
   (`ConstantFoldInstruction`, using the module's `DataLayout` and `TargetLibraryInfo`) and
   replaces their uses with the result.
2. **Dead-code elimination** - erases instructions that have no uses and no side effects
   (`isInstructionTriviallyDead`), walking each block bottom-up so dead chains disappear in one sweep.

The pass only edits instructions, never the CFG, so it reports `CFGAnalyses` as preserved.

## Build

Requires LLVM 18 (dev packages), Clang 18 and CMake >= 3.13.

```bash
# Ubuntu 24.04
sudo apt-get install llvm-18-dev clang-18 llvm-18 cmake

cmake -S . -B build -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm
cmake --build build          # produces build/FoldDCE.so
```

## Use it

`mem2reg` runs first so that `clang -O0` stack slots become SSA values the pass can see.

```bash
clang-18 -O0 -Xclang -disable-O0-optnone -S -emit-llvm tests/const_fold.c -o const_fold.ll
opt-18 -load-pass-plugin=build/FoldDCE.so -passes='mem2reg,fold-dce' \
       -fold-dce-verbose -S const_fold.ll -o const_fold.opt.ll
```

`-fold-dce-verbose` prints per-function statistics, e.g.
`fold-dce: @main: folded 4, erased 4 instruction(s) in 2 round(s)`.

## Example (`tests/const_fold.c`)

Before (after `mem2reg`):

```llvm
%1 = mul nsw i32 6, 7
%2 = add nsw i32 %1, 3
%3 = icmp sgt i32 %2, 40
...
%5 = shl i32 %2, 1
%9 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %2, i32 noundef %8)
```

After `fold-dce` - arithmetic is gone and `printf` receives the constants `45` and (via the phi) `90`:

```llvm
%4 = phi i32 [ 90, %1 ], [ 0, %2 ]
%5 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef 45, i32 noundef %4)
```

## Tests

`./tests/run_tests.sh` compiles each `tests/*.c` with Clang, runs the pass, then checks that
(a) the IR still passes the LLVM verifier, (b) the instruction count went down, and
(c) the program output (run with `lli`) is **identical** before and after.

Results on LLVM 18.1.3:

| Test         | Instr. before | Instr. after | Removed | Result |
|--------------|--------------:|-------------:|--------:|--------|
| `const_fold` | 10 | 6  | 4 | PASS |
| `dead_code`  | 5  | 2  | 3 | PASS |
| `mixed`      | 17 | 16 | 1 | PASS |

(Counts are textual IR instructions after `mem2reg`, with and without `fold-dce`.)

## Limitations

- **No control-flow simplification.** After folding a branch condition the pass leaves `br i1 true, ...`
  behind; running `simplifycfg` afterwards removes it (`-passes='mem2reg,fold-dce,simplifycfg'`).
- **No memory reasoning.** Values stored to memory are not propagated; use `mem2reg` first.
- **Scalar and local only.** No inter-procedural analysis; calls with possible side effects are kept.

## Layout

```
src/FoldDCEPass.cpp     the pass + plugin registration ("fold-dce")
tests/*.c               small C programs
tests/run_tests.sh      before/after IR size + output-equivalence checks
CMakeLists.txt
.github/workflows/ci.yml
```
