#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502INSTRUCTIONSELECTOR_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502INSTRUCTIONSELECTOR_H

#include "MOS6502TargetMachine.h"
#include "MOS6502RegisterBankInfo.h"
#include "MOS6502Subtarget.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"

namespace llvm {

InstructionSelector *
createMOS6502InstructionSelector(const MOS6502TargetMachine &TM,
                                 MOS6502Subtarget &STI,
                                 MOS6502RegisterBankInfo &RBI);

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502INSTRUCTIONSELECTOR_H
