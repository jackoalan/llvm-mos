#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502CALLINGCONV_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502CALLINGCONV_H

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/Support/MachineValueType.h"

namespace llvm {

bool CC_MOS6502(unsigned ValNo, MVT ValVT,
                MVT LocVT, CCValAssign::LocInfo LocInfo,
                ISD::ArgFlagsTy ArgFlags, CCState &State);

}  // namespace

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502CALLINGCONV_H
