#include "MOS6502RegisterInfo.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502InstrInfo.h"
#include "MOS6502Subtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/ErrorHandling.h"

#define DEBUG_TYPE "mos6502-reginfo"

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
  // Reserved for temporarily saving X/Y without clobbering A.
  Reserved.set(MOS6502::ZP_0);
  markSuperRegs(Reserved, MOS6502::ZP_0);
  return Reserved;
}

void MOS6502RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                              int SPAdj, unsigned FIOperandNum,
                                              RegScavenger *RS) const {
  const MachineFunction& MF = *MI->getMF();
  const MOS6502FrameLowering& TFI = *getFrameLowering(MF);

  MachineOperand &Op = MI->getOperand(FIOperandNum);

  Register FrameReg;
  StackOffset Offset = TFI.getFrameIndexReference(MF, Op.getIndex(), FrameReg);
  if (FrameReg != MOS6502::S)
    report_fatal_error("Soft stack not yet implemented.");
  Op.ChangeToImmediate(Offset.getFixed());
}

Register
MOS6502RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return MOS6502::S;
}

const uint32_t *
MOS6502RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                          CallingConv::ID) const {
  return MOS6502_NoRegs_RegMask;
}
