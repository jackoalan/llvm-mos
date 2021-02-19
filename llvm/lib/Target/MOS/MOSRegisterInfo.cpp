//===-- MOSRegisterInfo.cpp - MOS Register Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the MOS implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "MOSRegisterInfo.h"
#include "MCTargetDesc/MOSMCTargetDesc.h"
#include "MOSFrameLowering.h"
#include "MOSInstrInfo.h"
#include "MOSSubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/ErrorHandling.h"

#define DEBUG_TYPE "mos-reginfo"

#define GET_REGINFO_TARGET_DESC
#include "MOSGenRegisterInfo.inc"

using namespace llvm;

cl::opt<int>
    NumZPPtrs("num-zp-ptrs", cl::init(127),
              cl::desc("Number of zero-page pointers available for compiler "
                       "use, excluding the stack pointer."),
              cl::value_desc("zero-page pointers"));

MOSRegisterInfo::MOSRegisterInfo()
    : MOSGenRegisterInfo(/*RA=*/0, /*DwarfFlavor=*/0, /*EHFlavor=*/0,
                             /*PC=*/0, /*HwMode=*/0),
      ZPSymbolNames(new std::string[getNumRegs()]), Reserved(getNumRegs()) {
  for (unsigned Reg = 0; Reg < getNumRegs(); ++Reg) {
    // Pointers are referred to by their low byte in the addressing modes that
    // use them.
    unsigned R = Reg;
    if (MOS::ZP_PTRRegClass.contains(R))
      R = getSubReg(R, MOS::sublo);
    if (!MOS::ZPRegClass.contains(R))
      continue;
    ZPSymbolNames[Reg] = "_";
    ZPSymbolNames[Reg] += getName(R);
  }

  if (NumZPPtrs <= 1)
    report_fatal_error("At least two zero-page pointers must be available.");
  if (NumZPPtrs > 127)
    report_fatal_error("More than 127 zero-page pointers cannot be available.");

  for (int Idx = 0; Idx < 127 - NumZPPtrs; Idx++) {
    Reserved.set(MOS::ZP_PTR_126 - Idx);
    Reserved.set(MOS::ZP_253 - Idx * 2);
    Reserved.set(MOS::ZP_253 - Idx * 2 - 1);
  }

  // Stack pointers.
  Reserved.set(MOS::SP);
  Reserved.set(MOS::SPlo);
  Reserved.set(MOS::SPhi);
  Reserved.set(MOS::S);
  Reserved.set(MOS::Static);
}

const MCPhysReg *
MOSRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return MOS_CSR_SaveList;
}

const uint32_t *
MOSRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                          CallingConv::ID) const {
  return MOS_CSR_RegMask;
}

BitVector
MOSRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = getFrameLowering(MF);
  BitVector Reserved = this->Reserved;
  if (TFI->hasFP(MF)) {
    Reserved.set(MOS::ZP_PTR_1);
    Reserved.set(MOS::ZP_2);
    Reserved.set(MOS::ZP_3);
  }
  return Reserved;
}

unsigned MOSRegisterInfo::getCSRFirstUseCost() const {
  // A CSR save/restore is about 2.53 times more expensive than a hard stack load.
  // This with a denominator of 2^14 gives approximately 41506.
  return 41506;
}

const TargetRegisterClass *
MOSRegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
                                               const MachineFunction &) const {
  if (RC->contains(MOS::C))
    return &MOS::Anyi1RegClass;
  if (RC == &MOS::ZP_PTRRegClass)
    return RC;
  return &MOS::Anyi8RegClass;
}

void MOSRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                              int SPAdj, unsigned FIOperandNum,
                                              RegScavenger *RS) const {
  MachineFunction &MF = *MI->getMF();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const MOSFrameLowering &TFL = *getFrameLowering(MF);

  assert(!SPAdj);

  int Idx = MI->getOperand(FIOperandNum).getIndex();

  int64_t StackSize;
  Register Base;
  switch (MFI.getStackID(Idx)) {
  default:
    llvm_unreachable("Unexpected Stack ID");
  case TargetStackID::Default:
    StackSize = MFI.getStackSize();
    Base = getFrameRegister(MF);
    break;
  case TargetStackID::Hard:
    StackSize = TFL.hsSize(MFI);
    Base = MOS::S;
    break;
  case TargetStackID::NoAlloc:
    Base = MOS::Static;
    // Static stack grows up, so its offsets are positive.
    // Zeroing this allows the offsets through unchanged.
    StackSize = 0;
    break;
  }

  MI->getOperand(FIOperandNum).ChangeToRegister(Base, /*isDef=*/false);
  assert(MI->getOperand(FIOperandNum + 1).isImm());
  MI->getOperand(FIOperandNum + 1).setImm(StackSize + MFI.getObjectOffset(Idx));
}

Register
MOSRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = getFrameLowering(MF);
  return TFI->hasFP(MF) ? MOS::ZP_PTR_1 : MOS::SP;
}

bool MOSRegisterInfo::shouldCoalesce(
    MachineInstr *MI, const TargetRegisterClass *SrcRC, unsigned SubReg,
    const TargetRegisterClass *DstRC, unsigned DstSubReg,
    const TargetRegisterClass *NewRC, LiveIntervals &LIS) const {
  if (NewRC == &MOS::ZPRegClass &&
      (SrcRC == &MOS::AZPRegClass || DstRC == &MOS::AZPRegClass))
    return false;
  return true;
}
