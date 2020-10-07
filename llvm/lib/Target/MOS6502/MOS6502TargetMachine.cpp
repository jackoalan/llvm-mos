#include "MOS6502TargetMachine.h"
#include "TargetInfo/MOS6502TargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502Target() {
  RegisterTargetMachine<MOS6502TargetMachine> X(getTheMOS6502Target());
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
                      getEffectiveCodeModel(CM, CodeModel::Small), OL) {}
