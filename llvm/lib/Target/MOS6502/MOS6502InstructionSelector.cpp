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

  bool selectCompareBranch(MachineInstr &I, MachineRegisterInfo& MRI);

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

bool MOS6502InstructionSelector::select(MachineInstr &I) {
  if (!I.isPreISelOpcode())
    return true;
  if (selectImpl(I, *CoverageInfo))
    return true;

  MachineRegisterInfo &MRI = I.getParent()->getParent()->getRegInfo();

  switch(I.getOpcode()) {
  default:
    return false;
  case MOS6502::G_BRCOND:
    return selectCompareBranch(I, MRI);
  }
}

bool MOS6502InstructionSelector::selectCompareBranch(MachineInstr &I, MachineRegisterInfo &MRI) {
  assert(I.getOpcode() == MOS6502::G_BRCOND);
  Register CondReg = I.getOperand(0).getReg();
  MachineBasicBlock *Tgt = I.getOperand(1).getMBB();
  MachineInstr *CCMI = MRI.getVRegDef(CondReg);
  if (CCMI->getOpcode() != MOS6502::G_ICMP)
    return false;
  auto Pred = static_cast<CmpInst::Predicate>(CCMI->getOperand(1).getPredicate());
  if (Pred != CmpInst::ICMP_NE)
    return false;

  auto L = CCMI->getOperand(2).getReg();
  auto R = getConstantVRegValWithLookThrough(CCMI->getOperand(3).getReg(), MRI);
  if (!R)
    return false;

  MachineIRBuilder Builder(I);
  MachineInstrBuilder Compare = Builder.buildInstr(MOS6502::CMPimm).addUse(L).addImm(R->Value);
  if (!constrainSelectedInstRegOperands(*Compare, TII, TRI, RBI))
    return false;
  Builder.buildInstr(MOS6502::BNE).addMBB(Tgt);
  I.eraseFromParent();
  return true;
}

InstructionSelector *
llvm::createMOS6502InstructionSelector(const MOS6502TargetMachine &TM,
                                       MOS6502Subtarget &STI,
                                       MOS6502RegisterBankInfo &RBI) {
  return new MOS6502InstructionSelector(TM, STI, RBI);
}
