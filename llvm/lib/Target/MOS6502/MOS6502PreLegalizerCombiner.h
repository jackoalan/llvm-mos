#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502PRELEGALIZERCOMBINER_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502PRELEGALIZERCOMBINER_H

#include "llvm/Pass.h"

namespace llvm {

void initializeMOS6502PreLegalizerCombinerPass(PassRegistry&);
FunctionPass *createMOS6502PreLegalizerCombiner();

} // end namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502PRELEGALIZERCOMBINER_H
