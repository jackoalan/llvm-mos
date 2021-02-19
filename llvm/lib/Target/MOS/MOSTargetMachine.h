//===-- MOSTargetMachine.h - Define TargetMachine for MOS -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the MOS specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MOS_MOSTARGETMACHINE_H
#define LLVM_LIB_TARGET_MOS_MOSTARGETMACHINE_H

#include "MOSSubtarget.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class MOSTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  std::unique_ptr<MOSSubtarget> Subtarget;

 public:
  MOSTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                       CodeGenOpt::Level OL, bool JIT);

  const MOSSubtarget *getSubtargetImpl(const Function &F) const override;

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }

  TargetTransformInfo getTargetTransformInfo(const Function &F) override;

  void adjustPassManager(PassManagerBuilder &) override;
};

}  // namespace llvm


#endif  // not LLVM_LIB_TARGET_MOS_MOSTARGETMACHINE_H
