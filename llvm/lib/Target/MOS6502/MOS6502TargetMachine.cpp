#include "MOS6502TargetMachine.h"
#include "TargetInfo/MOS6502TargetInfo.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelect.h"
#include "llvm/CodeGen/GlobalISel/Legalizer.h"
#include "llvm/CodeGen/GlobalISel/RegBankSelect.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502Target() {
  RegisterTargetMachine<MOS6502TargetMachine> X(getTheMOS6502Target());
  initializeGlobalISel(*PassRegistry::getPassRegistry());
}

static const char Layout[] =
  "e-p:16:8:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8";

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
                      getEffectiveCodeModel(CM, CodeModel::Small), OL) {
  initAsmInfo();
  setGlobalISel(true);

  // Prevents fallback to SelectionDAG by allowing direct aborts.
  setGlobalISelAbort(GlobalISelAbortMode::Enable);
}

namespace {

class MOS6502PassConfig : public TargetPassConfig {
public:
  MOS6502PassConfig(MOS6502TargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  MOS6502TargetMachine &getMOS6502TargetMachine() const {
    return getTM<MOS6502TargetMachine>();
  }

  bool addIRTranslator() override;
  bool addLegalizeMachineIR() override;
  bool addRegBankSelect() override;
  bool addGlobalInstructionSelect() override;
};

}  // namespace

TargetPassConfig *MOS6502TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MOS6502PassConfig(*this, PM);
}

bool MOS6502PassConfig::addIRTranslator() {
  addPass(new IRTranslator(getOptLevel()));
  return false;
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
