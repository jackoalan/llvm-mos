#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502COMBINER_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502COMBINER_H

#include "llvm/Pass.h"

namespace llvm {

FunctionPass *createMOS6502Combiner();

} // end namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502COMBINER_H
