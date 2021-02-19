#ifndef LLVM_LIB_TARGET_MOS_MOSCALLINGCONV_H
#define LLVM_LIB_TARGET_MOS_MOSCALLINGCONV_H

#include "MCTargetDesc/MOSMCTargetDesc.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/Support/MachineValueType.h"

namespace llvm {

bool CC_MOS(unsigned ValNo, MVT ValVT, MVT LocVT,
                CCValAssign::LocInfo LocInfo, ISD::ArgFlagsTy ArgFlags,
                CCState &State);

bool CC_MOS_VarArgs(unsigned ValNo, MVT ValVT, MVT LocVT,
                        CCValAssign::LocInfo LocInfo, ISD::ArgFlagsTy ArgFlags,
                        CCState &State);

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOSCALLINGCONV_H
