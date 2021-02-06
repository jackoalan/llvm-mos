#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502STATICSTACKALLOC_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502STATICSTACKALLOC_H

#include "llvm/Pass.h"

namespace llvm {

ModulePass *createMOS6502StaticStackAllocPass();

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502STATICSTACKALLOC_H
