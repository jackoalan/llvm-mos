#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H

#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "MOS6502GenInstrInfo.inc"

namespace llvm {

class MOS6502InstrInfo : public MOS6502GenInstrInfo {
public:
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg,
                   bool KillSrc) const override;

  std::pair<unsigned, unsigned>
  decomposeMachineOperandsTargetFlags(unsigned TF) const override;

  ArrayRef<std::pair<unsigned, const char *>>
  getSerializableDirectMachineOperandTargetFlags() const override;
};

namespace MOS6502 {

enum TOF {
  MO_NO_FLAGS = 0,
  MO_LO,
  MO_HI,
};

} // namespace MOS6502

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H
