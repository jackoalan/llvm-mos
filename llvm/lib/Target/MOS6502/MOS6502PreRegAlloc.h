#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502PREREGALLOC_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502PREREGALLOC_H

#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {

MachineFunctionPass *createMOS6502PreRegAlloc();

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502PREREGALLOC_H
