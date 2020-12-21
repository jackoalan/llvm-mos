#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502INDEXIV_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502INDEXIV_H

#include "llvm/Analysis/LoopPass.h"

namespace llvm {

LoopPass *createMOS6502IndexIVPass();

} // end namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502INDEXIV_H
