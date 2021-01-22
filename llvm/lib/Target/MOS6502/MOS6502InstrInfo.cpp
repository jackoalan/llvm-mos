#include "MOS6502InstrInfo.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502RegisterInfo.h"

#include "llvm/ADT/SparseBitVector.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502-instrinfo"

#define GET_INSTRINFO_CTOR_DTOR
#include "MOS6502GenInstrInfo.inc"

namespace {

bool isMaybeLive(MachineIRBuilder &Builder, Register Reg) {
  const auto &MBB = Builder.getMBB();
  return MBB.computeRegisterLiveness(
             MBB.getParent()->getSubtarget().getRegisterInfo(), Reg,
             Builder.getInsertPt()) != MachineBasicBlock::LQR_Dead;
}

// Retrieve the first free register of a given class. If none are free,
// returns the first register in the class. Should not be used on classes
// containing reserved registers or CSRs.
Register trivialScavenge(MachineIRBuilder &Builder,
                         const TargetRegisterClass &RegClass) {
  for (Register Reg : RegClass)
    if (!isMaybeLive(Builder, Reg))
      return Reg;
  return *RegClass.begin();
}

} // namespace

bool MOS6502InstrInfo::isReallyTriviallyReMaterializable(const MachineInstr &MI,
                                                         AAResults *AA) const {
  switch (MI.getOpcode()) {
  default:
    return TargetInstrInfo::isReallyTriviallyReMaterializable(MI, AA);
  // Rematerializable as LDimm_preserve pseudo.
  case MOS6502::LDimm:
    return true;
  }
}

MachineInstr *MOS6502InstrInfo::commuteInstructionImpl(MachineInstr &MI,
                                                       bool NewMI,
                                                       unsigned Idx1,
                                                       unsigned Idx2) const {
  // TODO: A version of this that doesn't modify register classes if NewMI.
  if (NewMI)
    report_fatal_error("Not yet implemented.");

  MachineFunction &MF = *MI.getMF();
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  switch (MI.getOpcode()) {
  default:
    LLVM_DEBUG(dbgs() << "Commute: " << MI);
    llvm_unreachable("Unexpected instruction commute.");
  case MOS6502::ADCzpr:
    break;
  }

  const auto NewRegClass =
      [&](Register Reg,
          const TargetRegisterClass *RC) -> const TargetRegisterClass * {
    for (MachineOperand &MO : MRI.reg_nodbg_operands(Reg)) {
      MachineInstr *UseMI = MO.getParent();
      if (UseMI == &MI)
        continue;
      unsigned OpNo = &MO - &UseMI->getOperand(0);
      RC = UseMI->getRegClassConstraintEffect(OpNo, RC, this, &TRI);
      if (!RC)
        return nullptr;
    }
    return RC;
  };

  const TargetRegisterClass *RegClass1 =
      getRegClass(MI.getDesc(), Idx1, &TRI, MF);
  const TargetRegisterClass *RegClass2 =
      getRegClass(MI.getDesc(), Idx2, &TRI, MF);
  Register Reg1 = MI.getOperand(Idx1).getReg();
  Register Reg2 = MI.getOperand(Idx2).getReg();
  const TargetRegisterClass *Reg1Class = nullptr;
  const TargetRegisterClass *Reg2Class = nullptr;
  if (Reg1.isVirtual()) {
    Reg1Class = NewRegClass(Reg1, RegClass2);
    if (!Reg1Class)
      return nullptr;
  }
  if (Reg1.isPhysical() && !RegClass2->contains(Reg1))
    return nullptr;
  if (Reg2.isVirtual()) {
    Reg2Class = NewRegClass(Reg2, RegClass1);
    if (!Reg2Class)
      return nullptr;
  }
  if (Reg2.isPhysical() && !RegClass1->contains(Reg2))
    return nullptr;

  // If this fails, make sure to get it out of the way before rewriting reg
  // classes.
  MachineInstr *CommutedMI =
      TargetInstrInfo::commuteInstructionImpl(MI, NewMI, Idx1, Idx2);
  if (!CommutedMI)
    return nullptr;

  if (Reg1Class)
    MRI.setRegClass(Reg1, Reg1Class);
  if (Reg2Class)
    MRI.setRegClass(Reg2, Reg2Class);
  return CommutedMI;
}

void MOS6502InstrInfo::reMaterialize(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator I,
                                     Register DestReg, unsigned SubIdx,
                                     const MachineInstr &Orig,
                                     const TargetRegisterInfo &TRI) const {
  // A trivially rematerialized LDimm must preserve NZ.
  if (Orig.getOpcode() == MOS6502::LDimm) {
    MachineIRBuilder Builder(MBB, I);
    // Note: Explicitly don't copy over implicit nz def.
    Builder.buildInstr(MOS6502::LDimm_preserve)
        .addDef(DestReg)
        .add(Orig.getOperand(1));
    return;
  }

  TargetInstrInfo::reMaterialize(MBB, I, DestReg, SubIdx, Orig, TRI);
}

unsigned MOS6502InstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  // Overestimate the size of each instruction to guarantee that any necessary
  // branches are relaxed.
  return 3;
}

bool MOS6502InstrInfo::findCommutedOpIndices(const MachineInstr &MI,
                                             unsigned &SrcOpIdx1,
                                             unsigned &SrcOpIdx2) const {
  assert(!MI.isBundle() &&
         "MOS6502InstrInfo::findCommutedOpIndices() can't handle bundles");

  const MCInstrDesc &MCID = MI.getDesc();
  if (!MCID.isCommutable())
    return false;

  assert(MI.getOpcode() == MOS6502::ADCzpr);

  if (!fixCommutedOpIndices(SrcOpIdx1, SrcOpIdx2, 2, 3))
    return false;

  if (!MI.getOperand(SrcOpIdx1).isReg() || !MI.getOperand(SrcOpIdx2).isReg())
    // No idea.
    return false;
  return true;
}

bool MOS6502InstrInfo::isBranchOffsetInRange(unsigned BranchOpc,
                                             int64_t BrOffset) const {
  switch (BranchOpc) {
  default:
    llvm_unreachable("Bad branch opcode");
  case MOS6502::BR:
    // BR range is [-128,127] starting from the PC location after the
    // instruction, which is two bytes after the start of the instruction.
    return -126 <= BrOffset && BrOffset <= 129;
  case MOS6502::JMP:
    return true;
  }
}

MachineBasicBlock *
MOS6502InstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("Bad branch opcode");
  case MOS6502::BR:
  case MOS6502::JMP:
    return MI.getOperand(0).getMBB();
  }
}

bool MOS6502InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                     MachineBasicBlock *&TBB,
                                     MachineBasicBlock *&FBB,
                                     SmallVectorImpl<MachineOperand> &Cond,
                                     bool AllowModify) const {
  auto I = MBB.getFirstTerminator();

  // No terminators, so falls through.
  if (I == MBB.end())
    return false;

  // Non-branch terminators cannot be analyzed.
  if (!I->isBranch())
    return true;

  // Analyze first branch.
  auto FirstBR = I++;
  if (FirstBR->isPreISelOpcode())
    return true;
  // First branch always forms true edge, whether conditional or unconditional.
  TBB = getBranchDestBlock(*FirstBR);
  if (FirstBR->isConditionalBranch()) {
    Cond.push_back(FirstBR->getOperand(1));
    Cond.push_back(FirstBR->getOperand(2));
  }

  // If there's no second branch, done.
  if (I == MBB.end())
    return false;

  // Cannot analyze branch followed by non-branch.
  if (!I->isBranch())
    return true;

  auto SecondBR = I++;

  // If more than two branches present, cannot analyze.
  if (I != MBB.end())
    return true;

  // Exactly two branches present.

  // Can only analyze conditional branch followed by unconditional branch.
  if (!SecondBR->isUnconditionalBranch())
    return true;

  // Second unconditional branch forms false edge.
  if (SecondBR->isPreISelOpcode())
    return true;
  FBB = getBranchDestBlock(*SecondBR);
  return false;
}

unsigned MOS6502InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                        int *BytesRemoved) const {
  // Since analyzeBranch succeeded, we know that the only terminators are
  // branches.

  unsigned NumRemoved = std::distance(MBB.getFirstTerminator(), MBB.end());
  if (BytesRemoved) {
    *BytesRemoved = 0;
    for (const MachineInstr &I : MBB.terminators())
      *BytesRemoved += getInstSizeInBytes(I);
  }
  MBB.erase(MBB.getFirstTerminator(), MBB.end());
  return NumRemoved;
}

unsigned MOS6502InstrInfo::insertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded) const {
  // Since analyzeBranch succeeded and any existing branches were removed, there
  // can be no terminators.

  MachineIRBuilder Builder(MBB, MBB.end());
  unsigned NumAdded = 0;
  if (BytesAdded)
    *BytesAdded = 0;

  // Unconditional branch target.
  auto *UBB = TBB;

  // Conditional branch.
  if (!Cond.empty()) {
    assert(TBB);
    assert(Cond.size() == 2);

    // The unconditional branch will be to the false branch (if any).
    UBB = FBB;

    auto BR = Builder.buildInstr(MOS6502::BR).addMBB(TBB);
    for (const MachineOperand &Op : Cond) {
      BR.add(Op);
    }
    ++NumAdded;
    if (BytesAdded)
      *BytesAdded += getInstSizeInBytes(*BR);
  }

  if (UBB) {
    auto JMP = Builder.buildInstr(MOS6502::JMP).addMBB(UBB);
    ++NumAdded;
    if (BytesAdded)
      *BytesAdded += getInstSizeInBytes(*JMP);
  }

  return NumAdded;
}

void MOS6502InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const DebugLoc &DL, MCRegister DestReg,
                                   MCRegister SrcReg, bool KillSrc) const {
  MachineIRBuilder Builder(MBB, MI);
  preserveAroundPseudoExpansion(
      Builder, [&]() { copyPhysRegNoPreserve(Builder, DestReg, SrcReg); });
}

void MOS6502InstrInfo::copyPhysRegNoPreserve(MachineIRBuilder &Builder,
                                             MCRegister DestReg,
                                             MCRegister SrcReg) const {
  if (DestReg == SrcReg)
    return;

  const TargetRegisterInfo &TRI =
      *Builder.getMF().getSubtarget().getRegisterInfo();

  const auto &areClasses = [&](const TargetRegisterClass &Dest,
                               const TargetRegisterClass &Src) {
    return Dest.contains(DestReg) && Src.contains(SrcReg);
  };

  if (areClasses(MOS6502::GPRRegClass, MOS6502::GPRRegClass)) {
    if (SrcReg == MOS6502::A) {
      assert(MOS6502::XYRegClass.contains(DestReg));
      Builder.buildInstr(MOS6502::TA_).addDef(DestReg);
    } else if (DestReg == MOS6502::A) {
      assert(MOS6502::XYRegClass.contains(SrcReg));
      Builder.buildInstr(MOS6502::T_A).addUse(SrcReg);
    } else {
      copyPhysRegNoPreserve(Builder, MOS6502::A, SrcReg);
      copyPhysRegNoPreserve(Builder, DestReg, MOS6502::A);
    }
  } else if (areClasses(MOS6502::ZPRegClass, MOS6502::GPRRegClass)) {
    Builder.buildInstr(MOS6502::STzpr).addDef(DestReg).addUse(SrcReg);
  } else if (areClasses(MOS6502::GPRRegClass, MOS6502::ZPRegClass)) {
    Builder.buildInstr(MOS6502::LDzpr).addDef(DestReg).addUse(SrcReg);
  } else if (areClasses(MOS6502::ZP_PTRRegClass, MOS6502::ZP_PTRRegClass)) {
    copyPhysRegNoPreserve(Builder, TRI.getSubReg(DestReg, MOS6502::sublo),
                          TRI.getSubReg(SrcReg, MOS6502::sublo));
    copyPhysRegNoPreserve(Builder, TRI.getSubReg(DestReg, MOS6502::subhi),
                          TRI.getSubReg(SrcReg, MOS6502::subhi));
  } else if (areClasses(MOS6502::ZPRegClass, MOS6502::ZPRegClass)) {
    Register Tmp = trivialScavenge(Builder, MOS6502::GPRRegClass);
    copyPhysRegNoPreserve(Builder, Tmp, SrcReg);
    copyPhysRegNoPreserve(Builder, DestReg, Tmp);
  } else {
    LLVM_DEBUG(dbgs() << TRI.getName(DestReg) << " <- " << TRI.getName(SrcReg)
                      << "\n");
    report_fatal_error("Unsupported physical register copy.");
  }
}

void MOS6502InstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool isKill, int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  MachinePointerInfo PtrInfo =
      MachinePointerInfo::getFixedStack(MF, FrameIndex);
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      PtrInfo, MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  MachineIRBuilder Builder(MBB, MI);
  unsigned Opcode;
  if ((SrcReg.isVirtual() &&
       MRI.getRegClass(SrcReg)->hasSuperClassEq(&MOS6502::Anyi8RegClass)) ||
      (SrcReg.isPhysical() && MOS6502::Anyi8RegClass.contains(SrcReg))) {
    Opcode = MOS6502::STstk;
  } else if ((SrcReg.isVirtual() && MRI.getRegClass(SrcReg)->hasSuperClassEq(
                                        &MOS6502::ZP_PTRRegClass)) ||
             (SrcReg.isPhysical() &&
              MOS6502::ZP_PTRRegClass.contains(SrcReg))) {
    Opcode = MOS6502::STPtrstk;
  } else {
    report_fatal_error("Not yet implemented.");
  }
  Builder.buildInstr(Opcode)
      .addReg(SrcReg, getKillRegState(isKill))
      .addFrameIndex(FrameIndex)
      .addMemOperand(MMO);
}

void MOS6502InstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
    int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  MachinePointerInfo PtrInfo =
      MachinePointerInfo::getFixedStack(MF, FrameIndex);
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      PtrInfo, MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  MachineIRBuilder Builder(MBB, MI);
  unsigned Opcode;
  if ((DestReg.isVirtual() &&
       MRI.getRegClass(DestReg)->hasSuperClassEq(&MOS6502::Anyi8RegClass)) ||
      (DestReg.isPhysical() && MOS6502::Anyi8RegClass.contains(DestReg))) {
    Opcode = MOS6502::LDstk;
  } else if ((DestReg.isVirtual() && MRI.getRegClass(DestReg)->hasSuperClassEq(
                                         &MOS6502::ZP_PTRRegClass)) ||
             (DestReg.isPhysical() &&
              MOS6502::ZP_PTRRegClass.contains(DestReg))) {
    Opcode = MOS6502::LDPtrstk;
  } else {
    report_fatal_error("Not yet implemented.");
  }
  Builder.buildInstr(Opcode)
      .addDef(DestReg)
      .addFrameIndex(FrameIndex)
      .addMemOperand(MMO);
}

bool MOS6502InstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  bool Changed;
  MachineIRBuilder Builder(MI);
  preserveAroundPseudoExpansion(
      Builder, [&]() { Changed = expandPostRAPseudoNoPreserve(Builder); });
  return Changed;
}

bool MOS6502InstrInfo::expandPostRAPseudoNoPreserve(
    MachineIRBuilder &Builder) const {
  const TargetRegisterInfo &TRI =
      *Builder.getMF().getSubtarget().getRegisterInfo();

  auto &MI = *Builder.getInsertPt();
  bool Changed = true;
  switch (MI.getOpcode()) {
  default:
    Changed = false;
    break;
  case MOS6502::IncSP: {
    int64_t BytesImm = MI.getOperand(0).getImm();
    assert(BytesImm);
    assert(-32768 <= BytesImm && BytesImm < 32768);
    auto Bytes = static_cast<uint16_t>(BytesImm);
    auto LoBytes = Bytes & 0xFF;
    auto HiBytes = Bytes >> 8;
    assert(LoBytes || HiBytes);

    Builder.buildInstr(MOS6502::LDCimm).addDef(MOS6502::C).addImm(0);
    if (LoBytes) {
      Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::A).addImm(LoBytes);
      Builder.buildInstr(MOS6502::ADCzpr)
          .addDef(MOS6502::A)
          .addDef(MOS6502::C)
          .addUse(MOS6502::A)
          .addUse(MOS6502::SPlo)
          .addUse(MOS6502::C);
      Builder.buildInstr(MOS6502::STzpr)
          .addDef(MOS6502::SPlo)
          .addUse(MOS6502::A);
    }
    Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::A).addImm(HiBytes);
    Builder.buildInstr(MOS6502::ADCzpr)
        .addDef(MOS6502::A)
        .addDef(MOS6502::C, RegState::Dead)
        .addUse(MOS6502::A)
        .addUse(MOS6502::SPhi)
        .addUse(MOS6502::C);
    Builder.buildInstr(MOS6502::STzpr).addDef(MOS6502::SPhi).addUse(MOS6502::A);
    break;
  }

  case MOS6502::LDindirimm: {
    Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::Y).add(MI.getOperand(2));
    Builder.buildInstr(MOS6502::LDyindirr)
        .addDef(MOS6502::A)
        .add(MI.getOperand(1))
        .addUse(MOS6502::Y);
    copyPhysRegNoPreserve(Builder, MI.getOperand(0).getReg(), MOS6502::A);
    break;
  }

  case MOS6502::STindirimm: {
    copyPhysRegNoPreserve(Builder, MOS6502::A, MI.getOperand(0).getReg());
    Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::Y).add(MI.getOperand(2));
    Builder.buildInstr(MOS6502::STyindirr)
        .addUse(MOS6502::A)
        .add(MI.getOperand(1))
        .addUse(MOS6502::Y);
    break;
  }

  case MOS6502::AddrLoss: {
    int64_t OffsetImm = MI.getOperand(2).getImm();
    assert(0 <= OffsetImm && OffsetImm < 65536);
    auto Offset = static_cast<uint16_t>(OffsetImm);

    Offset = Offset & 0xFF;

    Register Dst = MI.getOperand(0).getReg();

    if (!Offset) {
      copyPhysRegNoPreserve(Builder, Dst, MOS6502::SPlo);
      break;
    }

    Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::A).addImm(Offset);
    Builder.buildInstr(MOS6502::LDCimm).addDef(MOS6502::C).addImm(0);
    Builder.buildInstr(MOS6502::ADCzpr)
        .addDef(MOS6502::A)
        .addDef(MOS6502::C)
        .addUse(MOS6502::A)
        .addUse(MOS6502::SPlo)
        .addUse(MOS6502::C);
    copyPhysRegNoPreserve(Builder, Dst, MOS6502::A);
    break;
  }

  case MOS6502::AddrHiss: {
    int64_t OffsetImm = MI.getOperand(1).getImm();
    assert(0 <= OffsetImm && OffsetImm < 65536);
    auto Offset = static_cast<uint16_t>(OffsetImm);

    Register Dst = MI.getOperand(0).getReg();

    if (!Offset) {
      copyPhysRegNoPreserve(Builder, Dst, MOS6502::SPhi);
      break;
    }

    Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::A).addImm(Offset >> 8);
    // AddrLoss won't reset the carry if it has a zero offset.
    if (!(Offset & 0xFF))
      Builder.buildInstr(MOS6502::LDCimm).addDef(MOS6502::C).addImm(0);
    Builder.buildInstr(MOS6502::ADCzpr)
        .addDef(MOS6502::A)
        .addDef(MOS6502::C, RegState::Dead)
        .addUse(MOS6502::A)
        .addUse(MOS6502::SPhi)
        .addUse(MOS6502::C);
    copyPhysRegNoPreserve(Builder, Dst, MOS6502::A);
    break;
  }

  case MOS6502::Addrss:
  case MOS6502::Addrhs: {
    unsigned LoOpcode = MI.getOpcode() == MOS6502::Addrss ? MOS6502::AddrLoss
                                                          : MOS6502::AddrLohs;
    unsigned HiOpcode = MI.getOpcode() == MOS6502::Addrss ? MOS6502::AddrHiss
                                                          : MOS6502::AddrHihs;

    Register Dst = MI.getOperand(0).getReg();
    const MachineOperand &Offset = MI.getOperand(1);
    auto End = Builder.getInsertPt();
    auto Lo = Builder.buildInstr(LoOpcode)
                  .addDef(TRI.getSubReg(Dst, MOS6502::sublo))
                  .addDef(MOS6502::C)
                  .add(Offset);
    auto Hi = Builder.buildInstr(HiOpcode)
                  .addDef(TRI.getSubReg(Dst, MOS6502::subhi))
                  .add(Offset);
    Builder.setInsertPt(Builder.getMBB(), Lo);
    expandPostRAPseudoNoPreserve(Builder);
    Builder.setInsertPt(Builder.getMBB(), Hi);
    expandPostRAPseudoNoPreserve(Builder);
    Builder.setInsertPt(Builder.getMBB(), End);
    break;
  }

  case MOS6502::AddrLohs: {
    int64_t OffsetImm = MI.getOperand(2).getImm();
    assert(0 <= OffsetImm && OffsetImm < 256);
    auto Offset = static_cast<uint8_t>(OffsetImm);

    Builder.buildInstr(MOS6502::TSX);
    if (!Offset) {
      copyPhysRegNoPreserve(Builder, MI.getOperand(0).getReg(), MOS6502::X);
      break;
    }

    copyPhysRegNoPreserve(Builder, MOS6502::A, MOS6502::X);
    Builder.buildInstr(MOS6502::LDCimm).addDef(MOS6502::C).addImm(0);
    Builder.buildInstr(MOS6502::ADCimm)
        .addDef(MOS6502::A)
        .addDef(MOS6502::C)
        .addUse(MOS6502::A)
        .addImm(Offset)
        .addUse(MOS6502::C);
    copyPhysRegNoPreserve(Builder, MI.getOperand(0).getReg(), MOS6502::A);
    break;
  }

  case MOS6502::AddrHihs: {
    int64_t OffsetImm = MI.getOperand(1).getImm();
    assert(0 <= OffsetImm && OffsetImm < 256);
    auto Offset = static_cast<uint8_t>(OffsetImm);

    Register Dst = MI.getOperand(0).getReg();

    if (!Offset) {
      if (MOS6502::GPRRegClass.contains(Dst))
        Builder.buildInstr(MOS6502::LDimm).addDef(Dst).addImm(1);
      else {
        Register Tmp = trivialScavenge(Builder, MOS6502::GPRRegClass);
        Builder.buildInstr(MOS6502::LDimm).addDef(Tmp).addImm(1);
        copyPhysRegNoPreserve(Builder, Dst, Tmp);
      }
      break;
    }

    Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::A).addImm(1);
    Builder.buildInstr(MOS6502::ADCimm)
        .addDef(MOS6502::A)
        .addDef(MOS6502::C, RegState::Dead)
        .addUse(MOS6502::A)
        .addImm(0)
        .addUse(MOS6502::C);
    copyPhysRegNoPreserve(Builder, Dst, MOS6502::A);
    break;
  }

  case MOS6502::LDidx:
    // This occur when X or Y is both the destination and index register.
    // Since the 6502 has no instruction for this, use A as the destination
    // instead, then transfer to the real destination.
    if (MI.getOperand(0).getReg() == MI.getOperand(2).getReg()) {
      Builder.buildInstr(MOS6502::LDAidx)
          .add(MI.getOperand(1))
          .add(MI.getOperand(2));
      Builder.buildInstr(MOS6502::TA_).add(MI.getOperand(0));
      break;
    }

    switch (MI.getOperand(0).getReg()) {
    default:
      llvm_unreachable("Bad destination for LDidx.");
    case MOS6502::A:
      Builder.buildInstr(MOS6502::LDAidx)
          .add(MI.getOperand(1))
          .add(MI.getOperand(2));
      break;
    case MOS6502::X:
      Builder.buildInstr(MOS6502::LDXidx).add(MI.getOperand(1));
      break;
    case MOS6502::Y:
      Builder.buildInstr(MOS6502::LDYidx).add(MI.getOperand(1));
      break;
    }
    break;

  case MOS6502::LDimm_preserve:
    Builder.buildInstr(MOS6502::LDimm)
        .add(MI.getOperand(0))
        .add(MI.getOperand(1));
    break;

  case MOS6502::LDhs: {
    Register Dst = MI.getOperand(0).getReg();
    Register Tmp = Dst;
    if (!MOS6502::GPRRegClass.contains(Tmp))
      Tmp = trivialScavenge(Builder, MOS6502::GPRRegClass);

    Builder.buildInstr(MOS6502::TSX);
    auto Ld = Builder.buildInstr(MOS6502::LDidx)
                  .addDef(Tmp)
                  .addImm(0x100 + MI.getOperand(1).getImm())
                  .addReg(MOS6502::X);
    copyPhysRegNoPreserve(Builder, Dst, Tmp);
    auto End = Builder.getInsertPt();
    Builder.setInsertPt(Builder.getMBB(), Ld);
    expandPostRAPseudoNoPreserve(Builder);
    Builder.setInsertPt(Builder.getMBB(), End);
    break;
  }

  case MOS6502::SThs: {
    Register Src = MI.getOperand(0).getReg();
    copyPhysRegNoPreserve(Builder, MOS6502::A, Src);

    Builder.buildInstr(MOS6502::TSX);
    Builder.buildInstr(MOS6502::STidx)
        .addUse(MOS6502::A)
        .addImm(0x100 + MI.getOperand(1).getImm())
        .addReg(MOS6502::X);
    break;
  }

  case MOS6502::Push: {
    Register Src = MI.getOperand(0).getReg();
    copyPhysRegNoPreserve(Builder, MOS6502::A, Src);
    Builder.buildInstr(MOS6502::PHA);
    break;
  }
  case MOS6502::Pull: {
    Register Dest = MI.getOperand(0).getReg();
    Builder.buildInstr(MOS6502::PLA);
    copyPhysRegNoPreserve(Builder, Dest, MOS6502::A);
    break;
  }
  }

  if (Changed) {
    Builder.setInsertPt(Builder.getMBB(), std::next(Builder.getInsertPt()));
    MI.eraseFromParent();
  }
  return Changed;
}

bool MOS6502InstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 2);
  auto &Val = Cond[1];
  Val.setImm(!Val.getImm());
  // Success.
  return false;
}

std::pair<unsigned, unsigned>
MOS6502InstrInfo::decomposeMachineOperandsTargetFlags(unsigned TF) const {
  return std::make_pair(TF, 0u);
}

ArrayRef<std::pair<unsigned, const char *>>
MOS6502InstrInfo::getSerializableDirectMachineOperandTargetFlags() const {
  static const std::pair<unsigned, const char *> Flags[] = {
      {MOS6502::MO_LO, "lo"}, {MOS6502::MO_HI, "hi"}};
  return Flags;
}

void MOS6502InstrInfo::preserveAroundPseudoExpansion(
    MachineIRBuilder &Builder, std::function<void()> ExpandFn) const {
  MachineBasicBlock &MBB = Builder.getMBB();
  const TargetRegisterInfo &TRI =
      *MBB.getParent()->getSubtarget().getRegisterInfo();

  // Returns the locations modified by the given instruction.
  const auto GetWrites = [&](MachineInstr &MI) {
    SparseBitVector<> Writes;
    for (unsigned Reg = MCRegister::FirstPhysicalReg; Reg < TRI.getNumRegs();
         ++Reg) {
      if (MI.definesRegister(Reg, &TRI))
        Writes.set(Reg);
    }
    return Writes;
  };

  SparseBitVector<> MaybeLive;
  for (unsigned Reg = MCRegister::FirstPhysicalReg; Reg < TRI.getNumRegs();
       ++Reg) {
    if (isMaybeLive(Builder, Reg))
      MaybeLive.set(Reg);
  }

  SparseBitVector<> ExpectedWrites = GetWrites(*Builder.getInsertPt());

  // If begin was the first instruction, it may no longer be the first once
  // ExpandFn is called, so make a note of it.
  auto Begin = Builder.getInsertPt();
  bool WasBegin = Begin == MBB.begin();
  // Have begin point at the instruction before the inserted range.
  if (!WasBegin)
    --Begin;

  ExpandFn();

  // If Begin was the first instruction, get the real first instruction now
  // that ExpandFn has been called. Otherwise, advance Begin to the first
  // instruction.
  if (WasBegin)
    Begin = MBB.begin();
  else
    ++Begin;
  auto End = Builder.getInsertPt();

  // Determine the writes of the expansion region.
  SparseBitVector<> Writes;
  for (auto I = Begin; I != End; ++I)
    Writes |= GetWrites(*I);

  SparseBitVector<> Save = MaybeLive;
  Save &= Writes;
  Save.intersectWithComplement(ExpectedWrites);

  const auto RecordSaved = [&](Register Reg) {
    for (MCSubRegIterator SubReg(Reg, &TRI, /*IncludeSelf=*/true);
         SubReg.isValid(); ++SubReg) {
      Save.reset(*SubReg);
    }
  };

  // This code is intentionally very simplistic: that way it's easy to verify
  // that it's complete. Eventually, more efficient PHA/PLA can be emitted in
  // situations that can be proven safe. Ideally, saving/restoring here should
  // be rare; especially if save/restores are elided as a post-processing
  // step. Thus, having a simple working version of this forms a good baseline
  // that the rest of the compiler can rely on.

  // After the save sequence, issued, all registers and flags are in the same
  // state as before the sequence. After the restore sequence, all registers
  // and flags are in the same state as before the sequence, except those that
  // were saved, which have their value at time of save. The only memory
  // locations affected by either are _Save<Reg>, for Reg={P,A,X,Y}.

  Builder.setInsertPt(MBB, Begin);
  if (Save.test(MOS6502::N) || Save.test(MOS6502::Z) || Save.test(MOS6502::C)) {
    Builder.buildInstr(MOS6502::PHA);
    Builder.buildInstr(MOS6502::PHP);
    Builder.buildInstr(MOS6502::PLA);
    Builder.buildInstr(MOS6502::STabs)
        .addUse(MOS6502::A)
        .addExternalSymbol("_SaveP");
    Builder.buildInstr(MOS6502::PLA);
  }
  if (Save.test(MOS6502::A))
    Builder.buildInstr(MOS6502::STabs)
        .addUse(MOS6502::A)
        .addExternalSymbol("_SaveA");
  if (Save.test(MOS6502::X))
    Builder.buildInstr(MOS6502::STabs)
        .addUse(MOS6502::X)
        .addExternalSymbol("_SaveX");
  if (Save.test(MOS6502::Y))
    Builder.buildInstr(MOS6502::STabs)
        .addUse(MOS6502::Y)
        .addExternalSymbol("_SaveY");

  Builder.setInsertPt(MBB, End);
  if (Save.test(MOS6502::N) || Save.test(MOS6502::Z) || Save.test(MOS6502::C)) {
    // Note: This is particularly awful due to the requirement that the last
    // operation be a PLP. This means we have to get P onto the stack behind
    // the values of any registers that need to be saved to do so; hence the
    // indexed store behind the saves of X and A. That way, we can restore X
    // and A *before* P, preventing those restores from clobbering NZ.
    Builder.buildInstr(MOS6502::PHA);
    Builder.buildInstr(MOS6502::PHA);
    Builder.buildInstr(MOS6502::T_A).addUse(MOS6502::X);
    Builder.buildInstr(MOS6502::PHA);
    Builder.buildInstr(MOS6502::TSX);
    Builder.buildInstr(MOS6502::LDabs)
        .addDef(MOS6502::A)
        .addExternalSymbol("_SaveP");
    Builder.buildInstr(MOS6502::STidx)
        .addUse(MOS6502::A)
        .addImm(0x103) // Byte pushed by first PHA.
        .addUse(MOS6502::X);
    Builder.buildInstr(MOS6502::PLA);
    Builder.buildInstr(MOS6502::TA_).addDef(MOS6502::X);
    Builder.buildInstr(MOS6502::PLA);
    Builder.buildInstr(MOS6502::PLP);
    RecordSaved(MOS6502::P);
  }
  if (Save.test(MOS6502::A)) {
    Builder.buildInstr(MOS6502::PHP);
    Builder.buildInstr(MOS6502::LDabs)
        .addDef(MOS6502::A)
        .addExternalSymbol("_SaveA");
    Builder.buildInstr(MOS6502::PLP);
    RecordSaved(MOS6502::A);
  }
  if (Save.test(MOS6502::X)) {
    Builder.buildInstr(MOS6502::PHP);
    Builder.buildInstr(MOS6502::LDabs)
        .addDef(MOS6502::X)
        .addExternalSymbol("_SaveX");
    Builder.buildInstr(MOS6502::PLP);
    RecordSaved(MOS6502::X);
  }
  if (Save.test(MOS6502::Y)) {
    Builder.buildInstr(MOS6502::PHP);
    Builder.buildInstr(MOS6502::LDabs)
        .addDef(MOS6502::Y)
        .addExternalSymbol("_SaveY");
    Builder.buildInstr(MOS6502::PLP);
    RecordSaved(MOS6502::Y);
  }

  if (Save.count()) {
    for (Register Reg : Save)
      LLVM_DEBUG(dbgs() << "Unhandled saved register: " << TRI.getName(Reg)
                        << "\n");

    LLVM_DEBUG(dbgs() << "MaybeLive:\n");
    for (Register Reg : MaybeLive)
      LLVM_DEBUG(dbgs() << TRI.getName(Reg) << "\n");

    LLVM_DEBUG(dbgs() << "Writes:\n");
    for (Register Reg : Writes)
      LLVM_DEBUG(dbgs() << TRI.getName(Reg) << "\n");

    LLVM_DEBUG(dbgs() << "Expected Writes:\n");
    for (Register Reg : ExpectedWrites)
      LLVM_DEBUG(dbgs() << TRI.getName(Reg) << "\n");

    report_fatal_error("Cannot yet preserve register.");
  }
}
