#include "MOS6502Combiner.h"

#include "MOS6502.h"

#include "llvm/CodeGen/GlobalISel/Combiner.h"
#include "llvm/CodeGen/GlobalISel/CombinerHelper.h"
#include "llvm/CodeGen/GlobalISel/CombinerInfo.h"
#include "llvm/CodeGen/GlobalISel/GISelKnownBits.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetMachine.h"

#define DEBUG_TYPE "mos6502-prelegalizer-combiner"

using namespace llvm;

class MOS6502CombinerHelperState {
protected:
  CombinerHelper &Helper;

public:
  MOS6502CombinerHelperState(CombinerHelper &Helper) : Helper(Helper) {}
};

#define MOS6502COMBINERHELPER_GENCOMBINERHELPER_DEPS
#include "MOS6502GenGICombiner.inc"
#undef MOS6502COMBINERHELPER_GENCOMBINERHELPER_DEPS

namespace {
#define MOS6502COMBINERHELPER_GENCOMBINERHELPER_H
#include "MOS6502GenGICombiner.inc"
#undef MOS6502COMBINERHELPER_GENCOMBINERHELPER_H

class MOS6502CombinerInfo : public CombinerInfo {
  GISelKnownBits *KB;
  MachineDominatorTree *MDT;
  MOS6502GenCombinerHelperRuleConfig GeneratedRuleCfg;

public:
  MOS6502CombinerInfo(bool EnableOpt, bool OptSize, bool MinSize,
                      GISelKnownBits *KB, MachineDominatorTree *MDT)
      : CombinerInfo(/*AllowIllegalOps*/ true, /*ShouldLegalizeIllegal*/ false,
                     /*LegalizerInfo*/ nullptr, EnableOpt, OptSize, MinSize),
        KB(KB), MDT(MDT) {
    if (!GeneratedRuleCfg.parseCommandLineOption())
      report_fatal_error("Invalid rule identifier");
  }

  virtual bool combine(GISelChangeObserver &Observer, MachineInstr &MI,
                       MachineIRBuilder &B) const override;
};

bool MOS6502CombinerInfo::combine(GISelChangeObserver &Observer,
                                  MachineInstr &MI, MachineIRBuilder &B) const {
  CombinerHelper Helper(Observer, B, KB, MDT);
  MOS6502GenCombinerHelper Generated(GeneratedRuleCfg, Helper);
  return Generated.tryCombineAll(Observer, MI, B);
}

#define MOS6502COMBINERHELPER_GENCOMBINERHELPER_CPP
#include "MOS6502GenGICombiner.inc"
#undef MOS6502COMBINERHELPER_GENCOMBINERHELPER_CPP

// Pass boilerplate
// ================

class MOS6502Combiner : public MachineFunctionPass {
public:
  static char ID;

  MOS6502Combiner();

  StringRef getPassName() const override { return "MOS6502Combiner"; }

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // end anonymous namespace

void MOS6502Combiner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
  getSelectionDAGFallbackAnalysisUsage(AU);
  AU.addRequired<GISelKnownBitsAnalysis>();
  AU.addPreserved<GISelKnownBitsAnalysis>();
  AU.addRequired<MachineDominatorTree>();
  AU.addPreserved<MachineDominatorTree>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

MOS6502Combiner::MOS6502Combiner() : MachineFunctionPass(ID) {
  initializeMOS6502CombinerPass(*PassRegistry::getPassRegistry());
}

bool MOS6502Combiner::runOnMachineFunction(MachineFunction &MF) {
  if (MF.getProperties().hasProperty(
          MachineFunctionProperties::Property::FailedISel))
    return false;
  auto *TPC = &getAnalysis<TargetPassConfig>();
  const Function &F = MF.getFunction();
  bool EnableOpt =
      MF.getTarget().getOptLevel() != CodeGenOpt::None && !skipFunction(F);
  GISelKnownBits *KB = &getAnalysis<GISelKnownBitsAnalysis>().get(MF);
  MachineDominatorTree *MDT = &getAnalysis<MachineDominatorTree>();
  MOS6502CombinerInfo PCInfo(EnableOpt, F.hasOptSize(), F.hasMinSize(), KB,
                             MDT);
  Combiner C(PCInfo, TPC);
  return C.combineMachineInstrs(MF, /*CSEInfo*/ nullptr);
}

char MOS6502Combiner::ID = 0;
INITIALIZE_PASS_BEGIN(MOS6502Combiner, DEBUG_TYPE,
                      "Combine MOS6502 machine instrs", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(GISelKnownBitsAnalysis)
INITIALIZE_PASS_END(MOS6502Combiner, DEBUG_TYPE,
                    "Combine MOS6502 machine instrs", false, false)

namespace llvm {
FunctionPass *createMOS6502Combiner() { return new MOS6502Combiner; }
} // namespace llvm
