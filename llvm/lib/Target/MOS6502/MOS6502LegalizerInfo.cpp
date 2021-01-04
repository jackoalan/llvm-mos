#include "MOS6502LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
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

  // Constants

  getActionDefinitionsBuilder(G_IMPLICIT_DEF).legalFor({s1, s8, s16, p});

  getActionDefinitionsBuilder(G_CONSTANT)
      .legalFor({s1, s8})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder(G_GLOBAL_VALUE).legalFor({p});

  // Type Conversions

  getActionDefinitionsBuilder(G_INTTOPTR).legalFor({{p, s16}});
  getActionDefinitionsBuilder(G_PTRTOINT).legalFor({{s16, p}});

  // Scalar Operations

  getActionDefinitionsBuilder(G_MERGE_VALUES).legalFor({{s16, s8}});

  getActionDefinitionsBuilder(G_UNMERGE_VALUES).legalFor({{s8, s16}});

  // Integer Operations

  getActionDefinitionsBuilder({G_ADD, G_OR, G_XOR})
      .legalFor({s8})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder(
      {G_SDIV, G_SREM, G_UDIV, G_UREM, G_CTLZ_ZERO_UNDEF})
      .libcall();

  getActionDefinitionsBuilder(G_ICMP).legalFor({{s1, s8}});

  getActionDefinitionsBuilder(G_PTR_ADD).legalFor({{p, s8}}).customFor(
      {{p, s16}});

  getActionDefinitionsBuilder(G_UADDO).customFor({s8});
  getActionDefinitionsBuilder(G_UADDE).legalFor({s8});

  // Floating Point Operations

  getActionDefinitionsBuilder({G_FADD,       G_FSUB,
                               G_FMUL,       G_FDIV,
                               G_FMA,        G_FPOW,
                               G_FREM,       G_FCOS,
                               G_FSIN,       G_FLOG10,
                               G_FLOG,       G_FLOG2,
                               G_FEXP,       G_FEXP2,
                               G_FCEIL,      G_FFLOOR,
                               G_FMINNUM,    G_FMAXNUM,
                               G_FSQRT,      G_FRINT,
                               G_FNEARBYINT, G_INTRINSIC_ROUNDEVEN,
                               G_FPEXT,      G_FPTRUNC,
                               G_FPTOSI,     G_FPTOUI,
                               G_SITOFP,     G_UITOFP})
      .libcall();

  // Memory Operations

  getActionDefinitionsBuilder({G_LOAD, G_STORE})
      .legalFor({{s8, p}})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder({G_MEMCPY, G_MEMMOVE, G_MEMSET}).libcall();

  // Control Flow

  getActionDefinitionsBuilder(G_PHI).legalFor({s8});

  getActionDefinitionsBuilder(G_BRCOND).legalFor({s1});

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
  MachineOperand &Offset = MI.getOperand(2);

  MachineInstr *GlobalBase = getOpcodeDef(G_GLOBAL_VALUE, Base.getReg(), MRI);
  Optional<int64_t> ConstOffset = getConstantVRegVal(Offset.getReg(), MRI);

  // Fold constant offsets into global value operand.
  if (GlobalBase && ConstOffset) {
    const MachineOperand &Op = GlobalBase->getOperand(1);
    Builder.buildInstr(G_GLOBAL_VALUE)
        .add(Result)
        .addGlobalAddress(Op.getGlobal(), Op.getOffset() + *ConstOffset);
    MI.removeFromParent();
    return true;
  }

  // Adds of zero-extended values can instead use the legal 8-bit version of
  // G_PTR_ADD, with the goal of selecting indexed addressing modes.
  MachineInstr *ZExtOffset = getOpcodeDef(G_ZEXT, Offset.getReg(), MRI);
  if (ZExtOffset) {
    Helper.Observer.changingInstr(MI);
    Offset.setReg(ZExtOffset->getOperand(1).getReg());
    Helper.Observer.changedInstr(MI);
    return true;
  }

  // Generalized pointer additions must be lowered to 16-bit integer arithmetic.
  LLT s16 = LLT::scalar(16);
  Register PtrVal =
      Builder.buildPtrToInt(s16, MI.getOperand(1))->getOperand(0).getReg();
  Register Sum =
      Builder.buildAdd(s16, PtrVal, MI.getOperand(2))->getOperand(0).getReg();
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
