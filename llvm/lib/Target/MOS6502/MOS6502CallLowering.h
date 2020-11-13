#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502CALLLOWERING_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502CALLLOWERING_H

#include "llvm/CodeGen/GlobalISel/CallLowering.h"

namespace llvm {

class MOS6502CallLowering : public CallLowering {
public:
  MOS6502CallLowering(const llvm::TargetLowering *TL) : CallLowering(TL) {}

  bool lowerReturn(MachineIRBuilder &MIRBuiler, const Value *Val,
                   ArrayRef<Register> VRegs) const override;

  bool lowerFormalArguments(MachineIRBuilder &MIRBuilder, const Function &F,
                            ArrayRef<ArrayRef<Register>> VRegs) const override;
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502CALLLOWERING_H
