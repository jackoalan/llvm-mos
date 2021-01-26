#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502NORECURSE_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502NORECURSE_H

#include "llvm/Analysis/CallGraphSCCPass.h"

namespace llvm {

CallGraphSCCPass *createMOS6502NoRecursePass();

} // end namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502NORECURSE_H
