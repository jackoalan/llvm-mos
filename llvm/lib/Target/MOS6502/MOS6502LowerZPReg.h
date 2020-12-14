#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502LOWERZPREG_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502LOWERZPREG_H

#include "llvm/Pass.h"

namespace llvm {

ModulePass *createMOS6502LowerZPReg();

} // end namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502LOWERZPREG_H
