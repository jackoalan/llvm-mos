#include "MOS6502InstrInfo.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502RegisterInfo.h"

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "MOS6502GenInstrInfo.inc"

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

void MOS6502InstrInfo::reMaterialize(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator I,
                                     Register DestReg, unsigned SubIdx,
                                     const MachineInstr &Orig,
                                     const TargetRegisterInfo &TRI) const {
  MachineInstr *MI = MBB.getParent()->CloneMachineInstr(&Orig);
  MI->substituteRegister(MI->getOperand(0).getReg(), DestReg, SubIdx, TRI);
  // A trivially rematerialized LDimm must preserve NZ.
  if (MI->getOpcode() == MOS6502::LDimm) {
    MI->setDesc(get(MOS6502::LDimm_preserve));
  }
  MBB.insert(I, MI);
}

unsigned MOS6502InstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  // Overestimate the size of each instruction to guarantee that any necessary
  // branches are relaxed.
  return 3;
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
      Builder, [&]() { copyPhysRegImpl(Builder, DestReg, SrcReg); });
}

void MOS6502InstrInfo::copyPhysRegImpl(MachineIRBuilder &Builder,
                                       MCRegister DestReg,
                                       MCRegister SrcReg) const {
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
      copyPhysRegImpl(Builder, MOS6502::A, SrcReg);
      copyPhysRegImpl(Builder, DestReg, MOS6502::A);
    }
  } else if (areClasses(MOS6502::ZPRegClass, MOS6502::GPRRegClass)) {
    Builder.buildInstr(MOS6502::STzpr).addDef(DestReg).addUse(SrcReg);
  } else if (areClasses(MOS6502::GPRRegClass, MOS6502::ZPRegClass)) {
    Builder.buildInstr(MOS6502::LDzpr).addDef(DestReg).addUse(SrcReg);
  } else {
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
  if (SrcReg.isVirtual()) {
    if (!MRI.getRegClass(SrcReg)->hasSuperClassEq(&MOS6502::Anyi8RegClass))
      report_fatal_error("Not yet implemented.");
  } else {
    if (!MOS6502::Anyi8RegClass.contains(SrcReg))
      report_fatal_error("Not yet implemented.");
  }
  Builder.buildInstr(MOS6502::SThs)
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
  if (DestReg.isVirtual()) {
    if (!MRI.getRegClass(DestReg)->hasSuperClassEq(&MOS6502::Anyi8RegClass))
      report_fatal_error("Not yet implemented.");
  } else {
    if (!MOS6502::Anyi8RegClass.contains(DestReg))
      report_fatal_error("Not yet implemented.");
  }
  Builder.buildInstr(MOS6502::LDhs)
      .addDef(DestReg)
      .addFrameIndex(FrameIndex)
      .addMemOperand(MMO);
}

bool MOS6502InstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  MachineIRBuilder Builder(MI);
  bool Changed = false;
  preserveAroundPseudoExpansion(Builder, [&]() {
    switch (MI.getOpcode()) {
    case MOS6502::LDidx:
      // This occur when X or Y is both the destination and index register.
      // Since the 6502 has no instruction for this, use A as the destination
      // instead, then transfer to the real destination.
      if (MI.getOperand(0).getReg() == MI.getOperand(2).getReg()) {
        Builder.buildInstr(MOS6502::LDAidx)
            .add(MI.getOperand(1))
            .add(MI.getOperand(2));
        Builder.buildInstr(MOS6502::TA_).add(MI.getOperand(0));
        Changed = true;
        return;
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
      Changed = true;
      return;
    }
  });
  if (Changed)
    MI.eraseFromParent();
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

  // Returns whether a physreg could be live into the pseudo.
  const auto IsMaybeLive = [&](Register Reg) {
    return MBB.computeRegisterLiveness(
               MBB.getParent()->getSubtarget().getRegisterInfo(), Reg,
               Builder.getInsertPt()) != MachineBasicBlock::LQR_Dead;
  };

  // Returns the locations modified by the given instruction.
  const auto GetWrites = [&](MachineInstr &MI) {
    unsigned Writes = None;
    if (MI.modifiesRegister(MOS6502::NZ))
      Writes |= NZ;
    if (MI.modifiesRegister(MOS6502::A))
      Writes |= A;
    return Writes;
  };

  unsigned MaybeLive = None;
  if (IsMaybeLive(MOS6502::NZ))
    MaybeLive |= NZ;
  if (IsMaybeLive(MOS6502::A))
    MaybeLive |= A;

  unsigned ExpectedWrites = GetWrites(*Builder.getInsertPt());

  // If begin was the first instruction, it may no longer be the first once
  // ExpandFn is called, so make a note of it.
  auto Begin = Builder.getInsertPt();
  bool WasBegin = Begin == MBB.begin();
  if (!WasBegin)
    --Begin;

  ExpandFn();

  // If begin was the first instruction, get the real first instruction now that
  // ExpandFn has been called.
  if (WasBegin)
    Begin = MBB.begin();
  auto End = Builder.getInsertPt();

  // Determine the writes of the expansion region.
  unsigned Writes = None;
  for (auto I = Begin; I != End; ++I)
    Writes |= GetWrites(*I);

  unsigned Save = MaybeLive & Writes & ~ExpectedWrites;
  // Restoring A requires writing NZ in PLA.
  if (Save & A) {
    Writes |= NZ;
    Save = MaybeLive & Writes & ~ExpectedWrites;
  }

  Builder.setInsertPt(MBB, Begin);
  if (Save & NZ)
    Builder.buildInstr(MOS6502::PHP);
  if (Save & A)
    Builder.buildInstr(MOS6502::PHA);

  Builder.setInsertPt(MBB, End);
  if (Save & A)
    Builder.buildInstr(MOS6502::PLA);
  if (Save & NZ)
    Builder.buildInstr(MOS6502::PLP);
}
