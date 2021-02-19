#ifndef LLVM_LIB_TARGET_MOS_MOSNORECURSE_H
#define LLVM_LIB_TARGET_MOS_MOSNORECURSE_H

#include "llvm/Analysis/CallGraphSCCPass.h"

namespace llvm {

CallGraphSCCPass *createMOSNoRecursePass();

} // end namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOSNORECURSE_H
