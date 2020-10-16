#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502ISELLOWERING_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502ISELLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class MOS6502TargetLowering : public TargetLowering {
 public:
  MOS6502TargetLowering(const TargetMachine &TM) : TargetLowering(TM) {}
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502ISELLOWERING_H
