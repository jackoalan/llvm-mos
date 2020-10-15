#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502SUBTARGET_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502SUBTARGET_H

#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "MOS6502GenSubtargetInfo.inc"

namespace llvm {

class MOS6502Subtarget : public MOS6502GenSubtargetInfo {
 public:
  MOS6502Subtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS);
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502SUBTARGET_H
