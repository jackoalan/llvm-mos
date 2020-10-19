#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H

#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "MOS6502GenInstrInfo.inc"

namespace llvm {

class MOS6502InstrInfo : public MOS6502GenInstrInfo {};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H
