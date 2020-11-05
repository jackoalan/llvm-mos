#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502ISELLOWERING_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502ISELLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"

#include "llvm/Target/TargetMachine.h"

namespace llvm {

class MOS6502Subtarget;

class MOS6502TargetLowering : public TargetLowering {
 public:
  MOS6502TargetLowering(const TargetMachine &TM,
                        const MOS6502Subtarget& STI);

  MVT getRegisterTypeForCallingConv(LLVMContext &Context,
                                    CallingConv::ID CC, EVT VT) const override;

  unsigned getNumRegistersForCallingConv(LLVMContext &Context,
                                         CallingConv::ID CC,
                                         EVT VT) const override;
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502ISELLOWERING_H
