#include "MOS6502InstrInfo.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502RegisterInfo.h"
#include "MOS6502Subtarget.h"

#include "llvm/ADT/SparseBitVector.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502-instrinfo"

#define GET_INSTRINFO_CTOR_DTOR
#include "MOS6502GenInstrInfo.inc"

static bool isMaybeLive(MachineIRBuilder &Builder, Register Reg) {
  const auto &MBB = Builder.getMBB();
  return MBB.computeRegisterLiveness(
             MBB.getParent()->getSubtarget().getRegisterInfo(), Reg,
             Builder.getInsertPt()) != MachineBasicBlock::LQR_Dead;
}

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
  const TargetRegisterInfo &TRI =
      *Builder.getMF().getSubtarget().getRegisterInfo();

  const auto &areClasses = [&](const TargetRegisterClass &Dest,
                               const TargetRegisterClass &Src) {
    return Dest.contains(DestReg) && Src.contains(SrcReg);
  };

  auto Begin = Builder.getInsertPt();
  auto End = Begin;
  bool WasBegin = Begin == Builder.getMBB().begin();
  if (!WasBegin)
    --Begin;

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
    bool NoA = false;
    if (isMaybeLive(Builder, MOS6502::A)) {
      // Try to using X or Y over A to avoid saving A.
      if (!isMaybeLive(Builder, MOS6502::X)) {
        copyPhysRegNoPreserve(Builder, MOS6502::X, SrcReg);
        copyPhysRegNoPreserve(Builder, DestReg, MOS6502::X);
        NoA = true;
      } else if (!isMaybeLive(Builder, MOS6502::Y)) {
        copyPhysRegNoPreserve(Builder, MOS6502::Y, SrcReg);
        copyPhysRegNoPreserve(Builder, DestReg, MOS6502::Y);
        NoA = true;
      }
    }
    if (!NoA) {
      copyPhysRegNoPreserve(Builder, MOS6502::A, SrcReg);
      copyPhysRegNoPreserve(Builder, DestReg, MOS6502::A);
    }
  } else
    report_fatal_error("Unsupported physical register copy.");

  if (WasBegin)
    Begin = Builder.getMBB().begin();
  else
    ++Begin;
  Builder.setInsertPt(Builder.getMBB(), Begin);
  while (Builder.getInsertPt() != End)
    expandPostRAPseudoNoPreserve(Builder);
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
  bool Changed;
  MachineIRBuilder Builder(MI);
  preserveAroundPseudoExpansion(
      Builder, [&]() { Changed = expandPostRAPseudoNoPreserve(Builder); });
  return Changed;
}

bool MOS6502InstrInfo::expandPostRAPseudoNoPreserve(
    MachineIRBuilder &Builder) const {
  auto &MI = *Builder.getInsertPt();

  auto Begin = Builder.getInsertPt();
  auto End = std::next(Begin);
  bool WasBegin = Begin == Builder.getMBB().begin();
  if (!WasBegin)
    --Begin;

  bool ErasedMI = false;

  // Erase the starting pseudo.
  const auto EraseMI = [&]() {
    ErasedMI = true;
    MI.eraseFromParent();
  };

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

    Builder.buildInstr(MOS6502::LDCimm).addImm(0);
    if (LoBytes) {
      Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::A).addImm(LoBytes);
      Builder.buildInstr(MOS6502::ADCzpr)
          .addDef(MOS6502::A)
          .addUse(MOS6502::A)
          .addUse(MOS6502::SPlo);
      Builder.buildInstr(MOS6502::STzpr)
          .addDef(MOS6502::SPlo)
          .addUse(MOS6502::A);
    }
    Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::A).addImm(HiBytes);
    Builder.buildInstr(MOS6502::ADCzpr)
        .addDef(MOS6502::A)
        .addUse(MOS6502::A)
        .addUse(MOS6502::SPhi);
    Builder.buildInstr(MOS6502::STzpr).addDef(MOS6502::SPhi).addUse(MOS6502::A);

    EraseMI();
    break;
  }

  case MOS6502::AddFiLo:
  case MOS6502::AdcFiHi: {
    int64_t OffsetImm = MI.getOperand(1).getImm();
    assert(0 <= OffsetImm && OffsetImm < 65536);
    auto Offset = static_cast<uint16_t>(OffsetImm);

    Register SP;
    bool ResetCarry = false;
    bool Copy = false;
    if (MI.getOpcode() == MOS6502::AddFiLo) {
      SP = MOS6502::SPlo;
      ResetCarry = true;
      Offset = Offset & 0xFF;
      Copy = !Offset;
    } else {
      SP = MOS6502::SPhi;
      // AddFiLo won't reset the carry if it has a zero offset.
      ResetCarry = !(Offset & 0xFF);
      Copy = !OffsetImm;
      Offset = Offset >> 8;
    }

    if (Copy) {
      copyPhysRegNoPreserve(Builder, MI.getOperand(0).getReg(), SP);
      EraseMI();
      break;
    }

    Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::A).addImm(Offset);
    if (ResetCarry)
      Builder.buildInstr(MOS6502::LDCimm).addImm(0);
    Builder.buildInstr(MOS6502::ADCzpr)
        .addDef(MOS6502::A)
        .addUse(MOS6502::A)
        .addUse(SP);
    if (MI.getOperand(0).getReg() != MOS6502::A)
      copyPhysRegNoPreserve(Builder, MI.getOperand(0).getReg(), MOS6502::A);
    EraseMI();
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
      EraseMI();
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
    EraseMI();
    break;

  case MOS6502::LDimm_preserve:
    MI.setDesc(get(MOS6502::LDimm));
    break;

  case MOS6502::LDhs: {
    if (!MOS6502::GPRRegClass.contains(MI.getOperand(0).getReg()))
      report_fatal_error("Not yet implemented.");

    Builder.buildInstr(MOS6502::TSX);
    Builder.buildInstr(MOS6502::LDidx)
        .add(MI.getOperand(0))
        .addImm(0x100 + MI.getOperand(1).getImm())
        .addReg(MOS6502::X);
    EraseMI();
    break;
  }

  case MOS6502::SThs: {
    Register Src = MI.getOperand(0).getReg();
    if (Src != MOS6502::A)
      copyPhysRegNoPreserve(Builder, MOS6502::A, Src);

    Builder.buildInstr(MOS6502::TSX);
    Builder.buildInstr(MOS6502::STAidx)
        .addImm(0x100 + MI.getOperand(1).getImm())
        .addReg(MOS6502::X);
    EraseMI();
    break;
  }

  case MOS6502::ADCzpr:
    MI.setDesc(get(MOS6502::ADCzp));
    break;
  case MOS6502::ASL:
  case MOS6502::ROL:
    if (MI.getOperand(0).getReg() == MOS6502::A) {
      MI.RemoveOperand(1);
      MI.RemoveOperand(0);
      switch (MI.getOpcode()) {
      case MOS6502::ASL:
        MI.setDesc(get(MOS6502::ASLA));
        break;
      case MOS6502::ROL:
        MI.setDesc(get(MOS6502::ROLA));
        break;
      }
    } else {
      assert(MOS6502::ZPRegClass.contains(MI.getOperand(0).getReg()));
      MI.RemoveOperand(0);
      switch (MI.getOpcode()) {
      case MOS6502::ASL:
        MI.setDesc(get(MOS6502::ASLzp));
        break;
      case MOS6502::ROL:
        MI.setDesc(get(MOS6502::ROLzp));
        break;
      }
    }
    break;
  case MOS6502::LDzpr:
    MI.setDesc(get(MOS6502::LDzp));
    break;
  case MOS6502::LDAyindirr:
    MI.setDesc(get(MOS6502::LDAyindir));
    break;
  case MOS6502::STzpr:
    MI.setDesc(get(MOS6502::STzp));
    break;
  case MOS6502::STAyindirr:
    MI.setDesc(get(MOS6502::STAyindir));
    break;
  }

  if (Changed) {
    if (!ErasedMI) {
      const MOS6502RegisterInfo &TRI =
          *Builder.getMF().getSubtarget<MOS6502Subtarget>().getRegisterInfo();

      for (MachineOperand &MO : MI.operands()) {
        if (!MO.isReg() || MO.isImplicit())
          continue;
        Register Reg = MO.getReg();
        if (MOS6502::ZP_PTRRegClass.contains(Reg))
          Reg = TRI.getSubReg(Reg, MOS6502::sublo);
        if (!MOS6502::ZPRegClass.contains(Reg))
          continue;
        MO.ChangeToES(TRI.getZPSymbolName(Reg));
      }
    }

    if (WasBegin)
      Begin = Builder.getMBB().begin();
    else
      ++Begin;
    Builder.setInsertPt(Builder.getMBB(), Begin);
    while (Builder.getInsertPt() != End)
      expandPostRAPseudoNoPreserve(Builder);
  } else
    Builder.setInsertPt(Builder.getMBB(), End);
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

  // If Begin was the first instruction, get the real first instruction now that
  // ExpandFn has been called. Otherwise, advance Begin to the first
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
  // Restoring A, X, or Y writes NZ.
  if (Save.test(MOS6502::A) || Save.test(MOS6502::X) || Save.test(MOS6502::Y)) {
    Writes.set(MOS6502::NZ);
    Save = MaybeLive;
    Save &= Writes;
    Save.intersectWithComplement(ExpectedWrites);
  }

  if (Save.test(MOS6502::X) && Save.test(MOS6502::Z))
    report_fatal_error("Not yet implemented.");

  // Note that X/Y are saved using a reserved ZP register. This seems like a
  // high cost to pay, but consider the case where X needs to be saved, but the
  // value written to A needs to be live out of the pseudo. A standard TXA, PHA
  // pushes X to the stack just fine, but once the correct output value has been
  // written to A, there's no way to PLA, TAX without clobbering it. We could
  // try pushing it to the stack with PHA, but it's now on top of the stack, and
  // it would require an indexed load to retrieve it. This is all considerably
  // worse than just saving X/Y to a ZP reg. We'll likely be able to use the
  // reserved reg for other purposes as well, so it shouldn't be too much of a
  // burden on register allocation.

  const auto RecordSaved = [&](Register Reg) {
    for (MCSubRegIterator SubReg(Reg, &TRI, /*IncludeSelf=*/true);
         SubReg.isValid(); ++SubReg) {
      Save.reset(*SubReg);
    }
  };

  Builder.setInsertPt(MBB, Begin);
  if (Save.test(MOS6502::N) || Save.test(MOS6502::Z) || Save.test(MOS6502::C))
    Builder.buildInstr(MOS6502::PHP);
  if (Save.test(MOS6502::A))
    Builder.buildInstr(MOS6502::PHA);
  if (Save.test(MOS6502::X)) {
    Builder.buildInstr(MOS6502::STzp)
      .addExternalSymbol("_ZP_0")
      .addUse(MOS6502::X);
  }
  else if (Save.test(MOS6502::Y)) {
    Builder.buildInstr(MOS6502::STzp)
      .addExternalSymbol("_ZP_0")
      .addUse(MOS6502::Y);
  }

  Builder.setInsertPt(MBB, End);
  if (Save.test(MOS6502::A)) {
    Builder.buildInstr(MOS6502::PLA);
    RecordSaved(MOS6502::A);
  }
  if (Save.test(MOS6502::X)) {
    Builder.buildInstr(MOS6502::LDzp).addDef(MOS6502::X).addExternalSymbol("_ZP_0");
    RecordSaved(MOS6502::X);
  } else if (Save.test(MOS6502::Y)) {
    Builder.buildInstr(MOS6502::LDzp).addDef(MOS6502::Y).addExternalSymbol("_ZP_0");
    RecordSaved(MOS6502::Y);
  }
  if (Save.test(MOS6502::N) || Save.test(MOS6502::Z) || Save.test(MOS6502::C)) {
    Builder.buildInstr(MOS6502::PLP);
    RecordSaved(MOS6502::P);
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
