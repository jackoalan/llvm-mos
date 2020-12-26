#include "MOS6502RegisterInfo.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502Subtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_REGINFO_TARGET_DESC
#include "MOS6502GenRegisterInfo.inc"

using namespace llvm;

MOS6502RegisterInfo::MOS6502RegisterInfo()
    : MOS6502GenRegisterInfo(/*RA=*/0, /*DwarfFlavor=*/0, /*EHFlavor=*/0,
                             /*PC=*/0, /*HwMode=*/0) {}

const MCPhysReg *
MOS6502RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const MCPhysReg regs[] = {0};
  return regs;
}

BitVector
MOS6502RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  return Reserved;
}

void MOS6502RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                              int SPAdj, unsigned FIOperandNum,
                                              RegScavenger *RS) const {
  report_fatal_error("Not yet implemented.");
}

Register
MOS6502RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  report_fatal_error("Not yet implemented.");
}

const uint32_t *
MOS6502RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                          CallingConv::ID) const {
  return MOS6502_NoRegs_RegMask;
}
