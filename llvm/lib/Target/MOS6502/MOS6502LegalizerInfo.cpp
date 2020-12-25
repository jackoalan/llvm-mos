#include "MOS6502LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

MOS6502LegalizerInfo::MOS6502LegalizerInfo() {
  using namespace TargetOpcode;
  using namespace LegalityPredicates;
  using namespace LegalizeMutations;

  LLT s1 = LLT::scalar(1);
  LLT s8 = LLT::scalar(8);
  LLT s16 = LLT::scalar(16);
  LLT p = LLT::pointer(0, 16);

  getActionDefinitionsBuilder({G_ADD, G_OR, G_XOR})
      .legalFor({s8})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder(G_BRCOND).legalFor({s1});

  getActionDefinitionsBuilder(G_CONSTANT).legalFor({s1, s8}).clampScalar(0, s8, s8);

  getActionDefinitionsBuilder({G_SDIV,
                               G_SREM,
                               G_UDIV,
                               G_UREM,
                               G_CTLZ_ZERO_UNDEF,
                               G_FADD,
                               G_FSUB,
                               G_FMUL,
                               G_FDIV,
                               G_FMA,
                               G_FPOW,
                               G_FREM,
                               G_FCOS,
                               G_FSIN,
                               G_FLOG10,
                               G_FLOG,
                               G_FLOG2,
                               G_FEXP,
                               G_FEXP2,
                               G_FCEIL,
                               G_FFLOOR,
                               G_FMINNUM,
                               G_FMAXNUM,
                               G_FSQRT,
                               G_FRINT,
                               G_FNEARBYINT,
                               G_INTRINSIC_ROUNDEVEN,
                               G_FPEXT,
                               G_FPTRUNC,
                               G_FPTOSI,
                               G_FPTOUI,
                               G_SITOFP,
                               G_UITOFP,
                               G_MEMCPY,
                               G_MEMMOVE,
                               G_MEMSET})
      .libcall();

  getActionDefinitionsBuilder({G_GLOBAL_VALUE, G_IMPLICIT_DEF, G_PHI})
      .alwaysLegal();

  getActionDefinitionsBuilder(G_INTTOPTR).legalFor({{p, s16}}).unsupported();
  getActionDefinitionsBuilder(G_PTRTOINT).legalFor({{s16, p}}).unsupported();

  getActionDefinitionsBuilder(G_ICMP)
      .legalFor({{s1, s8}})
      .narrowScalarIf(typeIs(1, p), changeTo(1, s8))
      .clampScalar(1, s8, s8);

  getActionDefinitionsBuilder(G_LOAD).legalFor({{s8, p}}).clampScalar(0, s8,
                                                                      s8);

  getActionDefinitionsBuilder({G_MERGE_VALUES, G_SEXT}).legalFor({{s16, s8}});

  getActionDefinitionsBuilder(G_PTR_ADD).legalFor({{p, s8}}).customFor(
      {{p, s16}});

  getActionDefinitionsBuilder(G_UADDO).customFor({s8});
  getActionDefinitionsBuilder(G_UADDE).legalFor({s8});

  getActionDefinitionsBuilder(G_UNMERGE_VALUES).legalFor({{s8, s16}});

  computeTables();
}

bool MOS6502LegalizerInfo::legalizeCustom(LegalizerHelper &Helper,
                                          MachineInstr &MI) const {
  using namespace TargetOpcode;
  MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();

  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("Invalid opcode for custom legalization.");
  case G_PTR_ADD:
    return legalizePtrAdd(Helper, MRI, MI);
  case G_UADDO:
    return legalizeUAddO(Helper, MRI, MI);
  }
}

bool MOS6502LegalizerInfo::legalizePtrAdd(LegalizerHelper &Helper,
                                          MachineRegisterInfo &MRI,
                                          MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_PTR_ADD);

  MachineIRBuilder Builder(MI);

  MachineOperand &Result = MI.getOperand(0);

  MachineOperand &Base = MI.getOperand(1);
  MachineInstr *BaseDef = MRI.getVRegDef(Base.getReg());

  MachineOperand &Offset = MI.getOperand(2);
  MachineInstr *OffsetDef = MRI.getVRegDef(Offset.getReg());

  // Fold constant offsets into global value operand.
  if (BaseDef->getOpcode() == G_GLOBAL_VALUE &&
      OffsetDef->getOpcode() == G_CONSTANT) {
    const MachineOperand &Base = BaseDef->getOperand(1);
    const GlobalValue *GV = Base.getGlobal();
    int64_t Offset = OffsetDef->getOperand(1).getCImm()->getSExtValue();

    Builder.buildInstr(G_GLOBAL_VALUE)
        .add(Result)
        // Note: Base may already have an offset, so add that in.
        .addGlobalAddress(GV, Base.getOffset() + Offset);
    MI.removeFromParent();
    return true;
  }

  // Adds of zero-extended values can instead use the legal 8-bit version of
  // G_PTR_ADD, with the goal of selecting indexed addressing modes.
  if (OffsetDef->getOpcode() == G_ZEXT) {
    Helper.Observer.changingInstr(MI);
    Offset.setReg(OffsetDef->getOperand(1).getReg());
    Helper.Observer.changedInstr(MI);
    return true;
  }

  // Generalized pointer additions must be lowered to 16-bit integer arithmetic.
  LLT s16 = LLT::scalar(16);
  Register PtrVal = MRI.createGenericVirtualRegister(s16);
  Builder.buildPtrToInt(PtrVal, MI.getOperand(1));
  Register Sum = MRI.createGenericVirtualRegister(s16);
  Builder.buildAdd(Sum, PtrVal, MI.getOperand(2));
  Builder.buildIntToPtr(MI.getOperand(0), Sum);
  MI.removeFromParent();
  return true;
}

bool MOS6502LegalizerInfo::legalizeUAddO(LegalizerHelper &Helper,
                                         MachineRegisterInfo &MRI,
                                         MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_UADDO);

  MachineIRBuilder Builder(MI);
  auto CarryIn = Builder.buildConstant(LLT::scalar(1), 0).getReg(0);
  Builder.buildUAdde(MI.getOperand(0), MI.getOperand(1), MI.getOperand(2),
                     MI.getOperand(3), CarryIn);
  MI.removeFromParent();
  return true;
}
