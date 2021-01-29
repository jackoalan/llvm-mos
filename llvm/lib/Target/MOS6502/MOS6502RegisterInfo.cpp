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

  // Stack pointer.
  Reserved.set(MOS6502::SP);
  Reserved.set(MOS6502::SPlo);
  Reserved.set(MOS6502::SPhi);
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
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

  assert(!SPAdj);

  MachineOperand &Op = MI->getOperand(FIOperandNum);

  int64_t StackSize;
  auto StackID = MFI.getStackID(Op.getIndex());
  if (StackID == TargetStackID::Hard) {
    StackSize = TFL.hsSize(MFI);
  } else {
    assert(!StackID);
    StackSize = MFI.getStackSize();
  }

  unsigned Offset = StackSize + MFI.getObjectOffset(Op.getIndex());
  if (!StackID && Offset >= 256)
    report_fatal_error("16-bit SP offsets not yet implemented.");

  unsigned Opcode;
  switch (MI->getOpcode()) {
  default:
    llvm_unreachable("Unexpected instruction with frame index.");
  case MOS6502::AddrLostk:
    Opcode =
        StackID == TargetStackID::Hard ? MOS6502::AddrLohs : MOS6502::AddrLoss;
    break;
  case MOS6502::AddrHistk:
    Opcode =
        StackID == TargetStackID::Hard ? MOS6502::AddrHihs : MOS6502::AddrHiss;
    break;
  case MOS6502::Addrstk:
    Opcode = StackID == TargetStackID::Hard ? MOS6502::Addrhs : MOS6502::Addrss;
    break;
  case MOS6502::LDstk:
    if (!StackID) {
      MI->RemoveOperand(1);
      Op.getParent()->setDesc(TII.get(MOS6502::LDindirimm));
      MI->addOperand(MachineOperand::CreateReg(MOS6502::SP, /*isDef=*/false));
      MI->addOperand(MachineOperand::CreateImm(Offset));
      return;
    }
    Opcode = MOS6502::LDhs;
    break;
  case MOS6502::STstk:
    if (!StackID) {
      MI->RemoveOperand(1);
      Op.getParent()->setDesc(TII.get(MOS6502::STindirimm));
      MI->addOperand(MachineOperand::CreateReg(MOS6502::SP, /*isDef=*/false));
      MI->addOperand(MachineOperand::CreateImm(Offset));
      return;
    }
    Opcode = MOS6502::SThs;
    break;
  case MOS6502::LDPtrstk: {
    Register Dst = MI->getOperand(0).getReg();
    // MI becomes the low half of the load.
    MI->getOperand(0).setReg(getSubReg(Dst, MOS6502::sublo));
    MI->dropMemRefs(MF);

    if (!StackID) {
      if (Offset + 1 >= 256)
        report_fatal_error("16-bit SP offsets not yet implemented.");

      Op.getParent()->setDesc(TII.get(MOS6502::LDindirimm));
      MI->getOperand(1).ChangeToRegister(MOS6502::SP, /*isDef=*/false);
      MI->addOperand(MachineOperand::CreateImm(Offset));

      // Insert the high half of the load.
      MachineIRBuilder Builder(*MI->getParent(), std::next(MI));
      Builder.buildInstr(MOS6502::LDindirimm)
          .addDef(getSubReg(Dst, MOS6502::subhi))
          .addUse(MOS6502::SP)
          .addImm(Offset + 1);
      return;
    }

    Opcode = MOS6502::LDhs;

    // Insert the high half of the load.
    MachineIRBuilder Builder(*MI->getParent(), std::next(MI));
    Builder.buildInstr(MOS6502::LDhs)
        .addDef(getSubReg(Dst, MOS6502::subhi))
        .addImm(Offset + 1);
    break;
  }
  case MOS6502::STPtrstk: {
    Register Src = MI->getOperand(0).getReg();
    // MI becomes the low half of the store.
    MI->getOperand(0).setReg(getSubReg(Src, MOS6502::sublo));
    MI->dropMemRefs(MF);

    if (!StackID) {
      if (Offset + 1 >= 256)
        report_fatal_error("16-bit SP offsets not yet implemented.");

      Op.getParent()->setDesc(TII.get(MOS6502::STindirimm));
      MI->getOperand(1).ChangeToRegister(MOS6502::SP, /*isDef=*/false);
      MI->addOperand(MachineOperand::CreateImm(Offset));

      // Insert the high half of the store.
      MachineIRBuilder Builder(*MI->getParent(), std::next(MI));
      Builder.buildInstr(MOS6502::STindirimm)
          .addUse(getSubReg(Src, MOS6502::subhi))
          .addUse(MOS6502::SP)
          .addImm(Offset + 1);
      return;
    }

    Opcode = MOS6502::SThs;

    // Insert the high half of the store.
    MachineIRBuilder Builder(*MI->getParent(), std::next(MI));
    Builder.buildInstr(MOS6502::SThs)
        .addUse(getSubReg(Src, MOS6502::subhi))
        .addImm(Offset + 1);
    break;
  }
  }

  Op.getParent()->setDesc(TII.get(Opcode));
  Op.ChangeToImmediate(Offset);
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
