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

cl::opt<int>
    NumZPPtrs("num-zp-ptrs", cl::init(127),
              cl::desc("Number of zero-page pointers available for compiler "
                       "use, excluding the stack pointer."),
              cl::value_desc("zero-page pointers"));

MOS6502RegisterInfo::MOS6502RegisterInfo()
    : MOS6502GenRegisterInfo(/*RA=*/0, /*DwarfFlavor=*/0, /*EHFlavor=*/0,
                             /*PC=*/0, /*HwMode=*/0),
      ZPSymbolNames(new std::string[getNumRegs()]), Reserved(getNumRegs()) {
  for (unsigned Reg = 0; Reg < getNumRegs(); ++Reg) {
    // Pointers are referred to by their low byte in the addressing modes that
    // use them.
    unsigned R = Reg;
    if (MOS6502::ZP_PTRRegClass.contains(R))
      R = getSubReg(R, MOS6502::sublo);
    if (!MOS6502::ZPRegClass.contains(R))
      continue;
    ZPSymbolNames[Reg] = "_";
    ZPSymbolNames[Reg] += getName(R);
  }

  if (NumZPPtrs <= 0)
    report_fatal_error("At least one zero-page pointer must be available.");
  if (NumZPPtrs > 127)
    report_fatal_error("More than 127 zero-page pointers cannot be available.");

  for (int Idx = 0; Idx < 127 - NumZPPtrs; Idx++) {
    Reserved.set(MOS6502::ZP_PTR_126 - Idx);
    Reserved.set(MOS6502::ZP_253 - Idx * 2);
    Reserved.set(MOS6502::ZP_253 - Idx * 2 - 1);
  }

  // Stack pointers.
  Reserved.set(MOS6502::SP);
  Reserved.set(MOS6502::SPlo);
  Reserved.set(MOS6502::SPhi);
  Reserved.set(MOS6502::S);
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
  MachineFunction &MF = *MI->getMF();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const MOS6502FrameLowering &TFL = *getFrameLowering(MF);

  assert(!SPAdj);

  int Idx = MI->getOperand(FIOperandNum).getIndex();

  int64_t StackSize;
  auto StackID = MFI.getStackID(Idx);
  if (StackID == TargetStackID::Hard) {
    StackSize = TFL.hsSize(MFI);
  } else {
    assert(!StackID);
    StackSize = MFI.getStackSize();
  }

  MI->getOperand(FIOperandNum)
      .ChangeToRegister(StackID ? MOS6502::S : MOS6502::SP,
                        /*isDef=*/false);
  assert(MI->getOperand(FIOperandNum + 1).isImm());
  MI->getOperand(FIOperandNum + 1).setImm(StackSize + MFI.getObjectOffset(Idx));
}

Register
MOS6502RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  report_fatal_error("Not yet implemented.");
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
