#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502TARGETMACHINE_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502TARGETMACHINE_H

#include "llvm/Target/TargetMachine.h"

namespace llvm {

class MOS6502TargetMachine : public LLVMTargetMachine {
 public:
  MOS6502TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                       CodeGenOpt::Level OL, bool JIT);

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
};

}  // namespace llvm


#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502TARGETMACHINE_H
