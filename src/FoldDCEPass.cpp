//===- FoldDCEPass.cpp - Constant folding + dead-code elimination ---------===//
//
// A small out-of-tree LLVM function pass for the new pass manager.
//
// Per function it repeats, until nothing changes:
//   1. Constant folding: any instruction whose operands are constants is
//      evaluated at compile time and its uses are replaced with the result.
//   2. Dead-code elimination: instructions with no uses and no side effects
//      are erased (this also removes the folded instructions themselves and
//      anything that only fed them).
//
// Usage:
//   opt -load-pass-plugin=./FoldDCE.so -passes='mem2reg,fold-dce' -S in.ll
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Local.h"

using namespace llvm;

static cl::opt<bool>
    Verbose("fold-dce-verbose",
            cl::desc("Print per-function statistics for the fold-dce pass"),
            cl::init(false));

namespace {

struct FoldDCEPass : PassInfoMixin<FoldDCEPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    if (F.isDeclaration())
      return PreservedAnalyses::all();

    const DataLayout &DL = F.getParent()->getDataLayout();
    const TargetLibraryInfo &TLI = FAM.getResult<TargetLibraryAnalysis>(F);

    unsigned NumFolded = 0, NumErased = 0, Rounds = 0;
    bool Changed = false, RoundChanged;

    do {
      RoundChanged = false;
      ++Rounds;

      // Phase 1: constant folding.
      for (BasicBlock &BB : F) {
        for (Instruction &I : make_early_inc_range(BB)) {
          if (I.use_empty())
            continue; // nothing to replace; DCE handles it
          if (Constant *C = ConstantFoldInstruction(&I, DL, &TLI)) {
            I.replaceAllUsesWith(C);
            ++NumFolded;
            RoundChanged = true;
          }
        }
      }

      // Phase 2: dead-code elimination. Walk each block bottom-up so a chain
      // of dead instructions is removed in a single sweep.
      for (BasicBlock &BB : F) {
        for (Instruction &I : make_early_inc_range(reverse(BB))) {
          if (isInstructionTriviallyDead(&I, &TLI)) {
            I.eraseFromParent();
            ++NumErased;
            RoundChanged = true;
          }
        }
      }

      Changed |= RoundChanged;
    } while (RoundChanged);

    if (Verbose)
      errs() << "fold-dce: @" << F.getName() << ": folded " << NumFolded
             << ", erased " << NumErased << " instruction(s) in " << Rounds
             << " round(s)\n";

    // We only touch instructions, never the CFG.
    if (!Changed)
      return PreservedAnalyses::all();
    PreservedAnalyses PA;
    PA.preserveSet<CFGAnalyses>();
    return PA;
  }

  static bool isRequired() { return false; }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "FoldDCE", "0.1.0", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "fold-dce") {
                    FPM.addPass(FoldDCEPass());
                    return true;
                  }
                  return false;
                });
          }};
}
