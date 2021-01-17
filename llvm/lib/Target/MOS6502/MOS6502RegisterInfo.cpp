#include "MOS6502RegisterInfo.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502FrameLowering.h"
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
                             /*PC=*/0, /*HwMode=*/0),
      ZPSymbolNames(new std::string[getNumRegs()]) {
  for (unsigned Reg = 0; Reg < getNumRegs(); ++Reg) {
    if (!MOS6502::ZPRegClass.contains(Reg))
      continue;
    ZPSymbolNames[Reg] = "_";
    ZPSymbolNames[Reg] += getName(Reg);
  }
}

const MCPhysReg *
MOS6502RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return MOS6502_NoRegs_SaveList;
}

const uint32_t *
MOS6502RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                          CallingConv::ID) const {
  return MOS6502_NoRegs_RegMask;
}

BitVector
MOS6502RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // Stack pointer.
  Reserved.set(MOS6502::SP);
  Reserved.set(MOS6502::SPlo);
  Reserved.set(MOS6502::SPhi);

  return Reserved;
}

const TargetRegisterClass *
MOS6502RegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
                                               const MachineFunction &) const {
  if (RC->contains(MOS6502::C))
    return &MOS6502::Anyi1RegClass;
  if (RC == &MOS6502::ZP_PTRRegClass)
    return RC;
  return &MOS6502::Anyi8RegClass;
}

void MOS6502RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                              int SPAdj, unsigned FIOperandNum,
                                              RegScavenger *RS) const {
  const MachineFunction &MF = *MI->getMF();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const MOS6502FrameLowering &TFL = *getFrameLowering(MF);

  assert(!SPAdj);

  MachineOperand &Op = MI->getOperand(FIOperandNum);

  int64_t StackSize;
  switch (MI->getOpcode()) {
  case MOS6502::AddFiLo:
  case MOS6502::AdcFiHi: {
    if (MFI.getStackID(Op.getIndex()) == TargetStackID::Hard)
      report_fatal_error("Hard stack FrameAddr not yet supported.");
    StackSize = MFI.getStackSize();
    break;
  }
  case MOS6502::LDhs:
  case MOS6502::SThs: {
    if (MFI.getStackID(Op.getIndex()) != TargetStackID::Hard)
      report_fatal_error("Soft stack LD/ST not yet supported.");
    StackSize = TFL.hsSize(MFI);
    break;
  }
  }

  Op.ChangeToImmediate(StackSize + MFI.getObjectOffset(Op.getIndex()));
}

Register
MOS6502RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return MOS6502::S;
}

bool MOS6502RegisterInfo::shouldCoalesce(
    MachineInstr *MI, const TargetRegisterClass *SrcRC, unsigned SubReg,
    const TargetRegisterClass *DstRC, unsigned DstSubReg,
    const TargetRegisterClass *NewRC, LiveIntervals &LIS) const {
  if (NewRC == &MOS6502::ZPRegClass &&
      (SrcRC == &MOS6502::AZPRegClass || DstRC == &MOS6502::AZPRegClass))
    return false;
  return true;
}
