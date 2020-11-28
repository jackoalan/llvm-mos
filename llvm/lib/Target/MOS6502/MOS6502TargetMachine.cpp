#include "MOS6502TargetMachine.h"

#include "MOS6502PreLegalizerCombiner.h"
#include "MOS6502TargetObjectFile.h"
#include "MOS6502TargetTransformInfo.h"
#include "TargetInfo/MOS6502TargetInfo.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelect.h"
#include "llvm/CodeGen/GlobalISel/Legalizer.h"
#include "llvm/CodeGen/GlobalISel/RegBankSelect.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502Target() {
  RegisterTargetMachine<MOS6502TargetMachine> X(getTheMOS6502Target());
  initializeGlobalISel(*PassRegistry::getPassRegistry());
}

static const char Layout[] =
    "e-p:16:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8";

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (!RM.hasValue())
    return Reloc::Static;
  return *RM;
}

MOS6502TargetMachine::MOS6502TargetMachine(const Target &T, const Triple &TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Optional<Reloc::Model> RM,
                                           Optional<CodeModel::Model> CM,
                                           CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, Layout, TT, CPU, FS, Options,
                        getEffectiveRelocModel(RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<MOS6502TargetObjectFile>()) {
  initAsmInfo();
  setGlobalISel(true);

  Subtarget = std::make_unique<MOS6502Subtarget>(TT, CPU, FS, *this);

  // Prevents fallback to SelectionDAG by allowing direct aborts.
  setGlobalISelAbort(GlobalISelAbortMode::Enable);
}

const MOS6502Subtarget *
MOS6502TargetMachine::getSubtargetImpl(const Function &F) const {
  return Subtarget.get();
}

TargetTransformInfo
MOS6502TargetMachine::getTargetTransformInfo(const Function &F) {
  return TargetTransformInfo(MOS6502TTIImpl(this, F));
}

namespace {

class MOS6502PassConfig : public TargetPassConfig {
public:
  MOS6502PassConfig(MOS6502TargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  MOS6502TargetMachine &getMOS6502TargetMachine() const {
    return getTM<MOS6502TargetMachine>();
  }

  bool addPreISel() override;
  bool addIRTranslator() override;
  void addPreLegalizeMachineIR() override;
  bool addLegalizeMachineIR() override;
  bool addRegBankSelect() override;
  bool addGlobalInstructionSelect() override;
};

} // namespace

TargetPassConfig *MOS6502TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MOS6502PassConfig(*this, PM);
}

bool MOS6502PassConfig::addPreISel() {
  // Combine any odd instruction sequences produced by loop strength reduction.
  addPass(createInstructionCombiningPass());
  addPass(createAggressiveDCEPass());
  return true;
}

bool MOS6502PassConfig::addIRTranslator() {
  addPass(new IRTranslator(getOptLevel()));
  return false;
}

void MOS6502PassConfig::addPreLegalizeMachineIR() {
  addPass(createMOS6502PreLegalizerCombiner());
}

bool MOS6502PassConfig::addLegalizeMachineIR() {
  addPass(new Legalizer());
  return false;
}

bool MOS6502PassConfig::addRegBankSelect() {
  addPass(new RegBankSelect());
  return false;
}

bool MOS6502PassConfig::addGlobalInstructionSelect() {
  addPass(new InstructionSelect());
  return false;
}
