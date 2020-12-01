#include "MOS6502InstructionSelector.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502RegisterInfo.h"
#include "MOS6502Subtarget.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelectorImpl.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/RegisterBankInfo.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace MIPatternMatch;

#define DEBUG_TYPE "mos6502-isel"

namespace {

#define GET_GLOBALISEL_PREDICATE_BITSET
#include "MOS6502GenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATE_BITSET

class MOS6502InstructionSelector : public InstructionSelector {
public:
  MOS6502InstructionSelector(const MOS6502TargetMachine &TM,
                             MOS6502Subtarget &STI,
                             MOS6502RegisterBankInfo &RBI);

  bool select(MachineInstr &I) override;
  static const char *getName() { return DEBUG_TYPE; }

private:
  const MOS6502InstrInfo &TII;
  const MOS6502RegisterInfo &TRI;
  const MOS6502RegisterBankInfo &RBI;

  bool selectCompareBranch(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectGlobalValue(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectLoad(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectIntToPtr(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectMergeValues(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectPhi(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectPtrToInt(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectUAddE(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectUAddO(MachineInstr &I, MachineRegisterInfo &MRI);
  bool selectUnMergeValues(MachineInstr &I, MachineRegisterInfo &MRI);

  void buildCopy(MachineIRBuilder &Builder, Register Dst, Register Src,
                 MachineRegisterInfo &MRI);

  void composePtr(MachineIRBuilder &Builder, Register Dst, Register Lo,
                  Register Hi, MachineRegisterInfo &MRI);

  void constrainGenericOp(MachineInstr &MI, MachineRegisterInfo &MRI);

  void constrainOperandRegClass(MachineOperand &RegMO,
                                const TargetRegisterClass &RegClass,
                                MachineRegisterInfo &MRI);

  /// tblgen-erated 'select' implementation, used as the initial selector for
  /// the patterns that don't require complex C++.
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;

#define GET_GLOBALISEL_PREDICATES_DECL
#include "MOS6502GenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_DECL

#define GET_GLOBALISEL_TEMPORARIES_DECL
#include "MOS6502GenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_DECL
};

} // namespace

#define GET_GLOBALISEL_IMPL
#include "MOS6502GenGlobalISel.inc"
#undef GET_GLOBALISEL_IMPL

MOS6502InstructionSelector::MOS6502InstructionSelector(
    const MOS6502TargetMachine &TM, MOS6502Subtarget &STI,
    MOS6502RegisterBankInfo &RBI)
    : TII(*STI.getInstrInfo()), TRI(*STI.getRegisterInfo()), RBI(RBI),
#define GET_GLOBALISEL_PREDICATES_INIT
#include "MOS6502GenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_INIT
#define GET_GLOBALISEL_TEMPORARIES_INIT
#include "MOS6502GenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_INIT
{
}

static const TargetRegisterClass &getRegClassForType(LLT Ty) {
  switch (Ty.getSizeInBits()) {
  default:
    llvm_unreachable("Invalid type size.");
  case 1:
  case 8:
    return MOS6502::AnycRegClass;
  case 16:
    return MOS6502::ZP_PTRRegClass;
  }
}

bool MOS6502InstructionSelector::select(MachineInstr &I) {
  if (!I.isPreISelOpcode())
    return true;
  if (selectImpl(I, *CoverageInfo))
    return true;

  MachineRegisterInfo &MRI = I.getParent()->getParent()->getRegInfo();

  switch (I.getOpcode()) {
  default:
    return false;
  case MOS6502::G_BRCOND:
    return selectCompareBranch(I, MRI);
  case MOS6502::G_GLOBAL_VALUE:
    return selectGlobalValue(I, MRI);
  case MOS6502::G_INTTOPTR:
    return selectIntToPtr(I, MRI);
  case MOS6502::G_LOAD:
    return selectLoad(I, MRI);
  case MOS6502::G_MERGE_VALUES:
    return selectMergeValues(I, MRI);
  case MOS6502::G_PHI:
    return selectPhi(I, MRI);
  case MOS6502::G_PTRTOINT:
    return selectPtrToInt(I, MRI);
  case MOS6502::G_UADDE:
    return selectUAddE(I, MRI);
  case MOS6502::G_UADDO:
    return selectUAddO(I, MRI);
  case MOS6502::G_UNMERGE_VALUES:
    return selectUnMergeValues(I, MRI);
  }
}

bool MOS6502InstructionSelector::selectCompareBranch(MachineInstr &I,
                                                     MachineRegisterInfo &MRI) {
  Register CondReg = I.getOperand(0).getReg();
  MachineBasicBlock *Tgt = I.getOperand(1).getMBB();

  MachineInstr *CCMI = MRI.getVRegDef(CondReg);

  if (CCMI->getOpcode() != MOS6502::G_ICMP)
    return false;
  auto Pred =
      static_cast<CmpInst::Predicate>(CCMI->getOperand(1).getPredicate());
  if (Pred != CmpInst::ICMP_NE)
    return false;

  auto L = CCMI->getOperand(2).getReg();
  auto R = getConstantVRegValWithLookThrough(CCMI->getOperand(3).getReg(), MRI);
  if (!R)
    return false;

  MachineIRBuilder Builder(I);
  auto Compare = Builder.buildInstr(MOS6502::CMPimm).addUse(L).addImm(R->Value);
  if (!constrainSelectedInstRegOperands(*Compare, TII, TRI, RBI))
    return false;
  Builder.buildInstr(MOS6502::BNE).addMBB(Tgt);
  I.eraseFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectGlobalValue(MachineInstr &I,
                                                   MachineRegisterInfo &MRI) {
  Register Dst = I.getOperand(0).getReg();
  const GlobalValue *Global = I.getOperand(1).getGlobal();

  MachineIRBuilder Builder(I);
  LLT s8 = LLT::scalar(8);
  Register Lo = MRI.createGenericVirtualRegister(s8);
  auto LoImm =
      Builder.buildInstr(MOS6502::LDgvlo).addDef(Lo).addGlobalAddress(Global);
  if (!constrainSelectedInstRegOperands(*LoImm, TII, TRI, RBI))
    return false;
  Register Hi = MRI.createGenericVirtualRegister(s8);
  auto HiImm =
      Builder.buildInstr(MOS6502::LDgvhi).addDef(Hi).addGlobalAddress(Global);
  if (!constrainSelectedInstRegOperands(*HiImm, TII, TRI, RBI))
    return false;
  composePtr(Builder, Dst, Lo, Hi, MRI);
  I.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectIntToPtr(MachineInstr &I,
                                                MachineRegisterInfo &MRI) {
  MachineIRBuilder Builder(I);
  buildCopy(Builder, I.getOperand(0).getReg(), I.getOperand(1).getReg(), MRI);
  I.eraseFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectLoad(MachineInstr &I,
                                            MachineRegisterInfo &MRI) {
  Register Dst = I.getOperand(0).getReg();
  Register Addr = I.getOperand(1).getReg();

  MachineIRBuilder Builder(I);
  Builder.buildInstr(MOS6502::LDimm).addDef(MOS6502::Y).addImm(0);
  auto Load = Builder.buildInstr(MOS6502::LDAyindir).addUse(Addr);
  if (!constrainSelectedInstRegOperands(*Load, TII, TRI, RBI))
    return false;
  buildCopy(Builder, Dst, MOS6502::A, MRI);
  I.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectMergeValues(MachineInstr &I,
                                                   MachineRegisterInfo &MRI) {
  Register Dst = I.getOperand(0).getReg();
  Register Lo = I.getOperand(1).getReg();
  Register Hi = I.getOperand(2).getReg();

  MachineIRBuilder Builder(I);
  composePtr(Builder, Dst, Lo, Hi, MRI);
  I.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectPhi(MachineInstr &I,
                                           MachineRegisterInfo &MRI) {
  MachineIRBuilder Builder(I);

  auto Phi = Builder.buildInstr(MOS6502::PHI);
  for (MachineOperand &Op : I.operands())
    Phi.add(Op);
  constrainGenericOp(*Phi, MRI);
  I.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectPtrToInt(MachineInstr &I,
                                                MachineRegisterInfo &MRI) {
  MachineIRBuilder Builder(I);
  buildCopy(Builder, I.getOperand(0).getReg(), I.getOperand(1).getReg(), MRI);
  I.eraseFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectUAddE(MachineInstr &I,
                                             MachineRegisterInfo &MRI) {
  Register Sum = I.getOperand(0).getReg();
  Register CarryOut = I.getOperand(1).getReg();
  Register L = I.getOperand(2).getReg();
  Register R = I.getOperand(3).getReg();
  Register CarryIn = I.getOperand(4).getReg();

  auto RConst = getConstantVRegValWithLookThrough(R, MRI);
  if (!RConst)
    return false;

  MachineIRBuilder Builder(I);
  auto SetC = Builder.buildInstr(MOS6502::SETC).addUse(CarryIn);
  if (!constrainSelectedInstRegOperands(*SetC, TII, TRI, RBI))
    return false;

  buildCopy(Builder, MOS6502::A, L, MRI);
  Builder.buildInstr(MOS6502::ADCimm).addImm(RConst->Value);
  buildCopy(Builder, Sum, MOS6502::A, MRI);

  auto GetC = Builder.buildInstr(MOS6502::GETC).addDef(CarryOut);
  if (!constrainSelectedInstRegOperands(*GetC, TII, TRI, RBI))
    return false;

  I.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectUAddO(MachineInstr &I,
                                             MachineRegisterInfo &MRI) {
  Register Sum = I.getOperand(0).getReg();
  Register CarryOut = I.getOperand(1).getReg();
  Register L = I.getOperand(2).getReg();
  Register R = I.getOperand(3).getReg();

  MachineIRBuilder Builder(I);

  Register CarryIn = MRI.createGenericVirtualRegister(LLT::scalar(1));
  auto LdCarry = Builder.buildInstr(MOS6502::LDimm).addDef(CarryIn).addImm(0);
  if (!constrainSelectedInstRegOperands(*LdCarry, TII, TRI, RBI))
    return false;
  auto Add = Builder.buildUAdde(Sum, CarryOut, L, R, CarryIn);

  I.removeFromParent();
  return selectUAddE(*Add, MRI);
}

bool MOS6502InstructionSelector::selectUnMergeValues(MachineInstr &I,
                                                     MachineRegisterInfo &MRI) {
  Register Lo = I.getOperand(0).getReg();
  Register Hi = I.getOperand(1).getReg();
  Register Src = I.getOperand(2).getReg();

  MachineIRBuilder Builder(I);
  constrainGenericOp(I, MRI);
  Builder.buildInstr(MOS6502::EXTRACT_SUBREG)
      .addDef(Lo)
      .addUse(Src)
      .addImm(MOS6502::sublo);
  Builder.buildInstr(MOS6502::EXTRACT_SUBREG)
      .addDef(Hi)
      .addUse(Src)
      .addImm(MOS6502::subhi);
  I.removeFromParent();
  return true;
}

void MOS6502InstructionSelector::buildCopy(MachineIRBuilder &Builder,
                                           Register Dst, Register Src,
                                           MachineRegisterInfo &MRI) {
  auto Copy = Builder.buildCopy(Dst, Src);
  constrainGenericOp(*Copy, MRI);
}

void MOS6502InstructionSelector::composePtr(MachineIRBuilder &Builder,
                                            Register Dst, Register Lo,
                                            Register Hi,
                                            MachineRegisterInfo &MRI) {
  auto RegSeq = Builder.buildInstr(MOS6502::REG_SEQUENCE)
                    .addDef(Dst)
                    .addUse(Lo)
                    .addImm(MOS6502::sublo)
                    .addUse(Hi)
                    .addImm(MOS6502::subhi);
  constrainGenericOp(*RegSeq, MRI);
}

void MOS6502InstructionSelector::constrainGenericOp(MachineInstr &MI,
                                                    MachineRegisterInfo &MRI) {
  for (MachineOperand &Op : MI.operands()) {
    if (!Op.isReg() || !Op.isDef() || Op.getReg().isPhysical())
      continue;
    LLT Ty = MRI.getType(Op.getReg());
    if (Ty.isPointer()) {
      Ty = LLT::scalar(16);
      MRI.setType(Op.getReg(), Ty);
    }
    constrainOperandRegClass(Op, getRegClassForType(Ty), MRI);
  }
}

void MOS6502InstructionSelector::constrainOperandRegClass(
    MachineOperand &RegMO, const TargetRegisterClass &RegClass,
    MachineRegisterInfo &MRI) {
  MachineInstr &MI = *RegMO.getParent();
  RegMO.setReg(llvm::constrainOperandRegClass(*MF, TRI, MRI, TII, RBI, MI,
                                              RegClass, RegMO));
}

InstructionSelector *
llvm::createMOS6502InstructionSelector(const MOS6502TargetMachine &TM,
                                       MOS6502Subtarget &STI,
                                       MOS6502RegisterBankInfo &RBI) {
  return new MOS6502InstructionSelector(TM, STI, RBI);
}
