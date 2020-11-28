#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502MACHINELEGALIZER_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502MACHINELEGALIZER_H

#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"

namespace llvm {

class MOS6502LegalizerInfo : public LegalizerInfo {
public:
  MOS6502LegalizerInfo();

  bool legalizeCustom(LegalizerHelper &Helper, MachineInstr &MI) const override;
};

} // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502MACHINELEGALIZER_H
