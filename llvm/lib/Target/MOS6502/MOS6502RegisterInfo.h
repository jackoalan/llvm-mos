#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERINFO_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "MOS6502GenRegisterInfo.inc"

namespace llvm {

struct MOS6502RegisterInfo : public MOS6502GenRegisterInfo {
  MOS6502RegisterInfo();

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  void eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERINFO_H
