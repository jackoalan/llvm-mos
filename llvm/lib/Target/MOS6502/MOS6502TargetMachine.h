#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502TARGETMACHINE_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502TARGETMACHINE_H

#include "MOS6502Subtarget.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class MOS6502TargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  std::unique_ptr<MOS6502Subtarget> Subtarget;

 public:
  MOS6502TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                       CodeGenOpt::Level OL, bool JIT);

  const MOS6502Subtarget *getSubtargetImpl(const Function &F) const override;

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }

  TargetTransformInfo getTargetTransformInfo(const Function &F) override;

  void adjustPassManager(PassManagerBuilder &) override;
};

}  // namespace llvm


#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502TARGETMACHINE_H
