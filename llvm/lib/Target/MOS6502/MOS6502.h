#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502_H

#include "llvm/Pass.h"

namespace llvm {

void initializeMOS6502IndexIVPass(PassRegistry &);
void initializeMOS6502PreRegAllocPass(PassRegistry &);
void initializeMOS6502PreLegalizerCombinerPass(PassRegistry &);

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502_H
