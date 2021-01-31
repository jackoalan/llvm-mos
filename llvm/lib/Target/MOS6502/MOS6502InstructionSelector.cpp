#include "MOS6502InstructionSelector.h"

#include <set>

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
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/ObjectYAML/MachOYAML.h"
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

  bool select(MachineInstr &MI) override;
  static const char *getName() { return DEBUG_TYPE; }

private:
  const MOS6502InstrInfo &TII;
  const MOS6502RegisterInfo &TRI;
  const MOS6502RegisterBankInfo &RBI;

  bool selectAdd(MachineInstr &MI);
  bool selectCompareBranch(MachineInstr &MI);
  bool selectConstant(MachineInstr &MI);
  bool selectFrameIndex(MachineInstr &MI);
  bool selectGlobalValue(MachineInstr &MI);
  bool selectLoad(MachineInstr &MI);
  bool selectLshO(MachineInstr &MI);
  bool selectLshE(MachineInstr &MI);
  bool selectImplicitDef(MachineInstr &MI);
  bool selectIntToPtr(MachineInstr &MI);
  bool selectMergeValues(MachineInstr &MI);
  bool selectPhi(MachineInstr &MI);
  bool selectPtrToInt(MachineInstr &MI);
  bool selectStore(MachineInstr &MI);
  bool selectUAddE(MachineInstr &MI);
  bool selectUnMergeValues(MachineInstr &MI);

  void buildCopy(MachineIRBuilder &Builder, Register Dst, Register Src);

  void composePtr(MachineIRBuilder &Builder, Register Dst, Register Lo,
                  Register Hi);

  void constrainGenericOp(MachineInstr &MI);

  void constrainOperandRegClass(MachineOperand &RegMO,
                                const TargetRegisterClass &RegClass);

  /// tblgen-erated 'select' implementation, used as the initial selector for
  /// the patterns that don't require complex C++.
  bool selectImpl(MachineInstr &MI, CodeGenCoverage &CoverageInfo) const;

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
    return MOS6502::Anyi1RegClass;
  case 8:
    return MOS6502::Anyi8RegClass;
  case 16:
    return MOS6502::ZP_PTRRegClass;
  }
}

bool MOS6502InstructionSelector::select(MachineInstr &MI) {
  if (!MI.isPreISelOpcode()) {
    // Copies can have generic VReg dests, so they must be constrained to a
    // register class.
    if (MI.isCopy())
      constrainGenericOp(MI);
    return true;
  }
  if (selectImpl(MI, *CoverageInfo))
    return true;

  switch (MI.getOpcode()) {
  default:
    return false;
  case MOS6502::G_ADD:
    return selectAdd(MI);
  case MOS6502::G_BRCOND:
    return selectCompareBranch(MI);
  case MOS6502::G_CONSTANT:
    return selectConstant(MI);
  case MOS6502::G_FRAME_INDEX:
    return selectFrameIndex(MI);
  case MOS6502::G_GLOBAL_VALUE:
    return selectGlobalValue(MI);
  case MOS6502::G_IMPLICIT_DEF:
    return selectImplicitDef(MI);
  case MOS6502::G_INTTOPTR:
    return selectIntToPtr(MI);
  case MOS6502::G_LOAD:
    return selectLoad(MI);
  case MOS6502::G_LSHO:
    return selectLshO(MI);
  case MOS6502::G_LSHE:
    return selectLshE(MI);
  case MOS6502::G_MERGE_VALUES:
    return selectMergeValues(MI);
  case MOS6502::G_PHI:
    return selectPhi(MI);
  case MOS6502::G_PTRTOINT:
    return selectPtrToInt(MI);
  case MOS6502::G_STORE:
    return selectStore(MI);
  case MOS6502::G_UADDE:
    return selectUAddE(MI);
  case MOS6502::G_UNMERGE_VALUES:
    return selectUnMergeValues(MI);
  }
}

bool MOS6502InstructionSelector::selectAdd(MachineInstr &MI) {
  MachineIRBuilder Builder(MI);
  Register CarryIn =
      Builder.getMRI()->createGenericVirtualRegister(LLT::scalar(1));
  Builder.buildInstr(MOS6502::LDCimm).addDef(CarryIn).addImm(0);
  auto Add = Builder.buildUAdde(MI.getOperand(0), LLT::scalar(1),
                                MI.getOperand(1), MI.getOperand(2), CarryIn);
  MI.eraseFromParent();
  if (!selectUAddE(*Add))
    return false;
  return true;
}

bool MOS6502InstructionSelector::selectCompareBranch(MachineInstr &MI) {
  MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();
  Register CondReg = MI.getOperand(0).getReg();
  MachineBasicBlock *Tgt = MI.getOperand(1).getMBB();

  CmpInst::Predicate Pred;
  Register LHS;
  int64_t RHS;
  if (!mi_match(CondReg, MRI, m_GICmp(m_Pred(Pred), m_Reg(LHS), m_ICst(RHS))))
    return false;

  MachineIRBuilder Builder(MI);

  auto Compare = Builder.buildInstr(MOS6502::CMPimm).addUse(LHS).addImm(RHS);
  if (!constrainSelectedInstRegOperands(*Compare, TII, TRI, RBI))
    return false;

  auto Br = Builder.buildInstr(MOS6502::BR).addMBB(Tgt);
  switch (Pred) {
  default:
    return false;
  case CmpInst::ICMP_EQ:
    Br.addUse(MOS6502::Z).addImm(1);
    break;
  case CmpInst::ICMP_NE:
    Br.addUse(MOS6502::Z).addImm(0);
    break;
  case CmpInst::ICMP_UGE:
    Br.addUse(MOS6502::N).addImm(0);
    break;
  case CmpInst::ICMP_ULT:
    Br.addUse(MOS6502::N).addImm(1);
    break;
  }
  MI.eraseFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectConstant(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_CONSTANT);

  MachineIRBuilder Builder(MI);
  // s8 is handled by TableGen LDimm.
  assert(Builder.getMRI()->getType(MI.getOperand(0).getReg()) ==
         LLT::scalar(1));
  auto Ld = Builder.buildInstr(MOS6502::LDCimm)
                .addDef(MI.getOperand(0).getReg())
                .addImm(MI.getOperand(1).getCImm()->getZExtValue());
  if (!constrainSelectedInstRegOperands(*Ld, TII, TRI, RBI))
    return false;
  MI.eraseFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectFrameIndex(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_FRAME_INDEX);

  MachineIRBuilder Builder(MI);
  MachineRegisterInfo &MRI = *Builder.getMRI();

  Register Dst = MI.getOperand(0).getReg();

  const auto AllUsesAreSubregs = [&]() {
    std::set<Register> WorkList = {Dst};
    std::vector<MachineOperand *> Uses;
    while (!WorkList.empty()) {
      Register Reg = *WorkList.begin();
      WorkList.erase(Reg);
      for (MachineOperand &MO : Builder.getMRI()->use_nodbg_operands(Reg)) {
        if (MO.getSubReg())
          continue;
        if (MO.getParent()->isCopy())
          WorkList.insert(MO.getParent()->getOperand(0).getReg());
        else
          return false;
      }
    }
    return true;
  };

  // Split the address into high and low halves to allow them to potentially be
  // allocated to GPR, maybe even to the same GPR at different times.
  if (AllUsesAreSubregs()) {
    LLT s8 = LLT::scalar(8);
    Register Lo = MRI.createGenericVirtualRegister(s8);
    Register Hi = MRI.createGenericVirtualRegister(s8);
    Register Carry = MRI.createGenericVirtualRegister(LLT::scalar(1));

    auto LoAddr = Builder.buildInstr(MOS6502::AddrLostk)
                      .addDef(Lo)
                      .addDef(Carry)
                      .add(MI.getOperand(1))
                      .addImm(0);
    if (!constrainSelectedInstRegOperands(*LoAddr, TII, TRI, RBI))
      return false;

    auto HiAddr = Builder.buildInstr(MOS6502::AddrHistk)
                      .addDef(Hi)
                      .add(MI.getOperand(1))
                      .addImm(0)
                      .addUse(Carry);
    if (!constrainSelectedInstRegOperands(*HiAddr, TII, TRI, RBI))
      return false;

    composePtr(Builder, Dst, Lo, Hi);
  } else {
    // At least one use is in a ZP_PTR, so don't break up the address. This
    // makes the operation easier to analyze and rematerialize.
    auto Addr = Builder.buildInstr(MOS6502::Addrstk)
                    .addDef(Dst)
                    .add(MI.getOperand(1))
                    .addImm(0);
    if (!constrainSelectedInstRegOperands(*Addr, TII, TRI, RBI))
      return false;
  }
  MI.eraseFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectGlobalValue(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_GLOBAL_VALUE);

  Register Dst = MI.getOperand(0).getReg();
  const GlobalValue *Global = MI.getOperand(1).getGlobal();

  MachineIRBuilder Builder(MI);
  MachineRegisterInfo &MRI = *Builder.getMRI();
  LLT s8 = LLT::scalar(8);
  Register Lo = MRI.createGenericVirtualRegister(s8);
  auto LoImm = Builder.buildInstr(MOS6502::LDimm)
                   .addDef(Lo)
                   .addGlobalAddress(Global, 0, MOS6502::MO_LO);
  if (!constrainSelectedInstRegOperands(*LoImm, TII, TRI, RBI))
    return false;
  Register Hi = MRI.createGenericVirtualRegister(s8);
  auto HiImm = Builder.buildInstr(MOS6502::LDimm)
                   .addDef(Hi)
                   .addGlobalAddress(Global, 0, MOS6502::MO_HI);
  if (!constrainSelectedInstRegOperands(*HiImm, TII, TRI, RBI))
    return false;
  composePtr(Builder, Dst, Lo, Hi);
  MI.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectImplicitDef(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_IMPLICIT_DEF);

  MachineIRBuilder Builder(MI);
  auto Def = Builder.buildInstr(MOS6502::IMPLICIT_DEF).add(MI.getOperand(0));
  constrainGenericOp(*Def);
  MI.eraseFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectIntToPtr(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_INTTOPTR);

  MachineIRBuilder Builder(MI);
  buildCopy(Builder, MI.getOperand(0).getReg(), MI.getOperand(1).getReg());
  MI.eraseFromParent();
  return true;
}

// Determines whether Addr can be referenced using the X/Y indexed addressing
// mode. If so, sets BaseOut to the base operand and Offset to the value that
// should be in X/Y.
static bool MatchIndexed(Register Addr, MachineOperand &BaseOut,
                         MachineOperand &OffsetOut,
                         const MachineRegisterInfo &MRI) {
  MachineInstr *SumAddr = getOpcodeDef(MOS6502::G_PTR_ADD, Addr, MRI);
  if (!SumAddr)
    return false;

  Register Base = SumAddr->getOperand(1).getReg();
  Register Offset = SumAddr->getOperand(2).getReg();

  MachineInstr *BaseGlobal = getOpcodeDef(MOS6502::G_GLOBAL_VALUE, Base, MRI);
  if (!BaseGlobal)
    return false;

  BaseOut = BaseGlobal->getOperand(1);
  // Constant offsets should already have been folded into the base.
  OffsetOut.ChangeToRegister(Offset, /*isDef=*/false);

  return true;
}

// Determines whether Addr can be referenced using the indirect-indexed (addr),Y
// addressing mode. If so, sets BaseOut to the base operand and Offset to the
// value that should be in Y.
static void MatchIndirectIndexed(Register Addr, MachineOperand &BaseOut,
                                 MachineOperand &OffsetOut,
                                 const MachineRegisterInfo &MRI) {
  MachineInstr *SumAddr = getOpcodeDef(MOS6502::G_PTR_ADD, Addr, MRI);
  if (!SumAddr) {
    BaseOut.ChangeToRegister(Addr, /*isDef=*/false);
    OffsetOut.ChangeToImmediate(0);
    return;
  }

  Register Base = SumAddr->getOperand(1).getReg();
  Register Offset = SumAddr->getOperand(2).getReg();

  BaseOut.ChangeToRegister(Base, /*isDef=*/false);
  auto ConstOffset = getConstantVRegValWithLookThrough(Offset, MRI);
  if (ConstOffset) {
    assert(ConstOffset->Value.getBitWidth() == 8);
    OffsetOut.ChangeToImmediate(ConstOffset->Value.getZExtValue());
  } else
    OffsetOut.ChangeToRegister(Offset, /*isDef=*/false);
}

bool MOS6502InstructionSelector::selectLoad(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_LOAD);

  Register Dst = MI.getOperand(0).getReg();
  Register Addr = MI.getOperand(1).getReg();
  MachineIRBuilder Builder(MI);
  MachineRegisterInfo &MRI = *Builder.getMRI();

  MachineOperand Base = MachineOperand::CreateImm(0);
  MachineOperand Offset = MachineOperand::CreateImm(0);
  if (MatchIndexed(Addr, Base, Offset, MRI)) {
    auto Load = Builder.buildInstr(MOS6502::LDidx)
                    .addDef(Dst)
                    .add(Base)
                    .add(Offset)
                    .cloneMemRefs(MI);
    if (!constrainSelectedInstRegOperands(*Load, TII, TRI, RBI))
      return false;
    MI.removeFromParent();
    return true;
  }

  MatchIndirectIndexed(Addr, Base, Offset, MRI);

  Register OffsetReg = MRI.createGenericVirtualRegister(LLT::scalar(8));
  if (Offset.isImm())
    Builder.buildInstr(MOS6502::LDimm).addDef(OffsetReg).add(Offset);
  else
    buildCopy(Builder, OffsetReg, Offset.getReg());

  auto Load = Builder.buildInstr(MOS6502::LDyindirr)
                  .addDef(Dst)
                  .addUse(Base.getReg())
                  .addUse(OffsetReg)
                  .cloneMemRefs(MI);
  if (!constrainSelectedInstRegOperands(*Load, TII, TRI, RBI))
    return false;

  MI.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectLshO(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_LSHO);

  Register Dst = MI.getOperand(0).getReg();
  Register CarryOut = MI.getOperand(1).getReg();
  Register Src = MI.getOperand(2).getReg();

  MachineIRBuilder Builder(MI);
  auto Asl =
      Builder.buildInstr(MOS6502::ASL).addDef(Dst).addDef(CarryOut).addUse(Src);
  if (!constrainSelectedInstRegOperands(*Asl, TII, TRI, RBI))
    return false;
  MI.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectLshE(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_LSHE);

  Register Dst = MI.getOperand(0).getReg();
  Register CarryOut = MI.getOperand(1).getReg();
  Register Src = MI.getOperand(2).getReg();
  Register CarryIn = MI.getOperand(3).getReg();

  MachineIRBuilder Builder(MI);
  auto Rol = Builder.buildInstr(MOS6502::ROL)
                 .addDef(Dst)
                 .addDef(CarryOut)
                 .addUse(Src)
                 .addUse(CarryIn);
  if (!constrainSelectedInstRegOperands(*Rol, TII, TRI, RBI))
    return false;
  MI.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectMergeValues(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_MERGE_VALUES);

  Register Dst = MI.getOperand(0).getReg();
  Register Lo = MI.getOperand(1).getReg();
  Register Hi = MI.getOperand(2).getReg();

  MachineIRBuilder Builder(MI);
  composePtr(Builder, Dst, Lo, Hi);
  MI.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectPhi(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_PHI);

  MachineIRBuilder Builder(MI);

  auto Phi = Builder.buildInstr(MOS6502::PHI);
  for (MachineOperand &Op : MI.operands())
    Phi.add(Op);
  constrainGenericOp(*Phi);
  MI.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectPtrToInt(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_PTRTOINT);

  MachineIRBuilder Builder(MI);
  buildCopy(Builder, MI.getOperand(0).getReg(), MI.getOperand(1).getReg());
  MI.eraseFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectStore(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_STORE);

  Register Src = MI.getOperand(0).getReg();
  Register Addr = MI.getOperand(1).getReg();
  MachineIRBuilder Builder(MI);
  MachineRegisterInfo &MRI = *Builder.getMRI();

  MachineOperand Base = MachineOperand::CreateImm(0);
  MachineOperand Offset = MachineOperand::CreateImm(0);
  if (MatchIndexed(Addr, Base, Offset, MRI)) {
    auto Store = Builder.buildInstr(MOS6502::STidx)
                     .addUse(Src)
                     .add(Base)
                     .add(Offset)
                     .cloneMemRefs(MI);
    if (!constrainSelectedInstRegOperands(*Store, TII, TRI, RBI))
      return false;
    MI.removeFromParent();
    return true;
  }

  MatchIndirectIndexed(Addr, Base, Offset, MRI);

  Register OffsetReg = MRI.createGenericVirtualRegister(LLT::scalar(8));
  if (Offset.isImm())
    Builder.buildInstr(MOS6502::LDimm).addDef(OffsetReg).add(Offset);
  else
    buildCopy(Builder, OffsetReg, Offset.getReg());

  auto Store = Builder.buildInstr(MOS6502::STyindirr)
                   .addUse(Src)
                   .addUse(Base.getReg())
                   .addUse(OffsetReg)
                   .cloneMemRefs(MI);
  if (!constrainSelectedInstRegOperands(*Store, TII, TRI, RBI))
    return false;

  MI.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectUAddE(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_UADDE);

  Register Sum = MI.getOperand(0).getReg();
  Register CarryOut = MI.getOperand(1).getReg();
  Register L = MI.getOperand(2).getReg();
  Register R = MI.getOperand(3).getReg();
  Register CarryIn = MI.getOperand(4).getReg();

  MachineIRBuilder Builder(MI);

  auto RConst = getConstantVRegValWithLookThrough(R, *Builder.getMRI());
  MachineInstrBuilder Add;
  if (RConst) {
    assert(RConst->Value.getBitWidth() == 8);
    Add = Builder.buildInstr(MOS6502::ADCimm)
              .addDef(Sum)
              .addDef(CarryOut)
              .addUse(L)
              .addImm(RConst->Value.getZExtValue())
              .addUse(CarryIn);
  } else {
    Add = Builder.buildInstr(MOS6502::ADCzpr)
              .addDef(Sum)
              .addDef(CarryOut)
              .addUse(L)
              .addUse(R)
              .addUse(CarryIn);
  }
  if (!constrainSelectedInstRegOperands(*Add, TII, TRI, RBI))
    return false;

  MI.removeFromParent();
  return true;
}

bool MOS6502InstructionSelector::selectUnMergeValues(MachineInstr &MI) {
  assert(MI.getOpcode() == MOS6502::G_UNMERGE_VALUES);

  Register Lo = MI.getOperand(0).getReg();
  Register Hi = MI.getOperand(1).getReg();
  Register Src = MI.getOperand(2).getReg();

  MachineIRBuilder Builder(MI);

  auto LoCopy = Builder.buildInstr(MOS6502::COPY)
                    .addDef(Lo)
                    .addUse(Src, 0, MOS6502::sublo);
  constrainGenericOp(*LoCopy);
  auto HiCopy = Builder.buildInstr(MOS6502::COPY)
                    .addDef(Hi)
                    .addUse(Src, 0, MOS6502::subhi);
  constrainGenericOp(*HiCopy);
  MI.removeFromParent();
  return true;
}

void MOS6502InstructionSelector::buildCopy(MachineIRBuilder &Builder,
                                           Register Dst, Register Src) {
  auto Copy = Builder.buildCopy(Dst, Src);
  constrainGenericOp(*Copy);
}

void MOS6502InstructionSelector::composePtr(MachineIRBuilder &Builder,
                                            Register Dst, Register Lo,
                                            Register Hi) {
  auto RegSeq = Builder.buildInstr(MOS6502::REG_SEQUENCE)
                    .addDef(Dst)
                    .addUse(Lo)
                    .addImm(MOS6502::sublo)
                    .addUse(Hi)
                    .addImm(MOS6502::subhi);
  constrainGenericOp(*RegSeq);

  // Propagate Lo and Hi to uses, hopefully killing the REG_SEQUENCE and
  // unconstraining the register classes of Lo and Hi.
  std::set<Register> WorkList = {Dst};
  std::vector<MachineOperand *> Uses;
  while (!WorkList.empty()) {
    Register Reg = *WorkList.begin();
    WorkList.erase(Reg);
    for (MachineOperand &MO : Builder.getMRI()->use_nodbg_operands(Reg)) {
      if (MO.getSubReg())
        Uses.push_back(&MO);
      else if (MO.getParent()->isCopy())
        WorkList.insert(MO.getParent()->getOperand(0).getReg());
    }
  }

  for (MachineOperand *MO : Uses) {
    if (MO->getSubReg() == MOS6502::sublo) {
      MO->setReg(Lo);
    } else {
      assert(MO->getSubReg() == MOS6502::subhi);
      MO->setReg(Hi);
    }
    MO->setSubReg(0);
  }
}

void MOS6502InstructionSelector::constrainGenericOp(MachineInstr &MI) {
  MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();
  for (MachineOperand &Op : MI.operands()) {
    if (!Op.isReg() || !Op.isDef() || Op.getReg().isPhysical())
      continue;
    LLT Ty = MRI.getType(Op.getReg());
    if (Ty.isPointer()) {
      Ty = LLT::scalar(16);
      MRI.setType(Op.getReg(), Ty);
    }
    constrainOperandRegClass(Op, getRegClassForType(Ty));
  }
}

void MOS6502InstructionSelector::constrainOperandRegClass(
    MachineOperand &RegMO, const TargetRegisterClass &RegClass) {
  MachineInstr &MI = *RegMO.getParent();
  MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();
  RegMO.setReg(llvm::constrainOperandRegClass(*MF, TRI, MRI, TII, RBI, MI,
                                              RegClass, RegMO));
}

InstructionSelector *
llvm::createMOS6502InstructionSelector(const MOS6502TargetMachine &TM,
                                       MOS6502Subtarget &STI,
                                       MOS6502RegisterBankInfo &RBI) {
  return new MOS6502InstructionSelector(TM, STI, RBI);
}
