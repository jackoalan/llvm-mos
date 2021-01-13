#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERINFO_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "MOS6502GenRegisterInfo.inc"

namespace llvm {

class MOS6502RegisterInfo : public MOS6502GenRegisterInfo {
  std::unique_ptr<std::string[]> ZPSymbolNames;

public:
  MOS6502RegisterInfo();

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  const TargetRegisterClass *
  getLargestLegalSuperClass(const TargetRegisterClass *RC,
                            const MachineFunction &) const override;

  void eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  const char* getZPSymbolName(Register Reg) const {
    return ZPSymbolNames[Reg].c_str();
  }
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERINFO_H
