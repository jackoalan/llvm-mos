#include "MOS6502PreLegalizerCombiner.h"

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

class MOS6502PreLegalizerCombinerHelperState {
protected:
  CombinerHelper &Helper;

public:
  MOS6502PreLegalizerCombinerHelperState(CombinerHelper &Helper)
      : Helper(Helper) {}
};

#define MOS6502PRELEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_DEPS
#include "MOS6502GenPreLegalizeGICombiner.inc"
#undef MOS6502PRELEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_DEPS

namespace {
#define MOS6502PRELEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_H
#include "MOS6502GenPreLegalizeGICombiner.inc"
#undef MOS6502PRELEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_H

class MOS6502PreLegalizerCombinerInfo : public CombinerInfo {
  GISelKnownBits *KB;
  MachineDominatorTree *MDT;
  MOS6502GenPreLegalizerCombinerHelperRuleConfig GeneratedRuleCfg;

public:
  MOS6502PreLegalizerCombinerInfo(bool EnableOpt, bool OptSize, bool MinSize,
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

bool MOS6502PreLegalizerCombinerInfo::combine(GISelChangeObserver &Observer,
                                              MachineInstr &MI,
                                              MachineIRBuilder &B) const {
  CombinerHelper Helper(Observer, B, KB, MDT);
  MOS6502GenPreLegalizerCombinerHelper Generated(GeneratedRuleCfg, Helper);
  return Generated.tryCombineAll(Observer, MI, B);
}

#define MOS6502PRELEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_CPP
#include "MOS6502GenPreLegalizeGICombiner.inc"
#undef MOS6502PRELEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_CPP

// Pass boilerplate
// ================

class MOS6502PreLegalizerCombiner : public MachineFunctionPass {
public:
  static char ID;

  MOS6502PreLegalizerCombiner();

  StringRef getPassName() const override {
    return "MOS6502PreLegalizerCombiner";
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // end anonymous namespace

void MOS6502PreLegalizerCombiner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
  getSelectionDAGFallbackAnalysisUsage(AU);
  AU.addRequired<GISelKnownBitsAnalysis>();
  AU.addPreserved<GISelKnownBitsAnalysis>();
  AU.addRequired<MachineDominatorTree>();
  AU.addPreserved<MachineDominatorTree>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

MOS6502PreLegalizerCombiner::MOS6502PreLegalizerCombiner()
    : MachineFunctionPass(ID) {
  initializeMOS6502PreLegalizerCombinerPass(*PassRegistry::getPassRegistry());
}

bool MOS6502PreLegalizerCombiner::runOnMachineFunction(MachineFunction &MF) {
  if (MF.getProperties().hasProperty(
          MachineFunctionProperties::Property::FailedISel))
    return false;
  auto *TPC = &getAnalysis<TargetPassConfig>();
  const Function &F = MF.getFunction();
  bool EnableOpt =
      MF.getTarget().getOptLevel() != CodeGenOpt::None && !skipFunction(F);
  GISelKnownBits *KB = &getAnalysis<GISelKnownBitsAnalysis>().get(MF);
  MachineDominatorTree *MDT = &getAnalysis<MachineDominatorTree>();
  MOS6502PreLegalizerCombinerInfo PCInfo(EnableOpt, F.hasOptSize(),
                                         F.hasMinSize(), KB, MDT);
  Combiner C(PCInfo, TPC);
  return C.combineMachineInstrs(MF, /*CSEInfo*/ nullptr);
}

char MOS6502PreLegalizerCombiner::ID = 0;
INITIALIZE_PASS_BEGIN(MOS6502PreLegalizerCombiner, DEBUG_TYPE,
                      "Combine MOS6502 machine instrs before legalization",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(GISelKnownBitsAnalysis)
INITIALIZE_PASS_END(MOS6502PreLegalizerCombiner, DEBUG_TYPE,
                    "Combine MOS6502 machine instrs before legalization", false,
                    false)

namespace llvm {
FunctionPass *createMOS6502PreLegalizerCombiner() {
  return new MOS6502PreLegalizerCombiner;
}
} // namespace llvm
